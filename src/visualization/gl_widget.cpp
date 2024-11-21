//
// Created by Raphael Russo on 11/20/24.
//

#include "gl_widget.h"
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMouseEvent>
#include <QWheelEvent>


namespace imu_viz {

    GLWidget::GLWidget(QWidget* parent)
            : QOpenGLWidget(parent)
            , program(new QOpenGLShaderProgram(this))
            , vbo(QOpenGLBuffer::VertexBuffer)
            , axesVBO(QOpenGLBuffer::VertexBuffer)
            , gridVBO(QOpenGLBuffer::VertexBuffer)
            , rotationSpeed(0.5f)
            , panSpeed(0.01f)
            , zoomSpeed(0.1f)
    {
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);

        // Initialize camera
        cameraPosition = QVector3D(0.0f, 0.0f, 5.0f);
        cameraTarget = QVector3D(0.0f, 0.0f, 0.0f);
        cameraUp = QVector3D(0.0f, 1.0f, 0.0f);

        // Initialize matrices
        model.setToIdentity();
        view.setToIdentity();
        projection.setToIdentity();

        updateCamera();  // Set initial view matrix
    }

    GLWidget::~GLWidget() {
        makeCurrent();
        vbo.destroy();
        axesVBO.destroy();
        gridVBO.destroy();
        doneCurrent();
    }

    void GLWidget::updateCamera() {
        // Store current distance and offset before updating
        float currentDistance = (cameraPosition - cameraTarget).length();
        QVector3D currentOffset = cameraTarget - QVector3D(0, 0, 0);  // Store target offset from origin

        // Calculate new camera position using spherical coordinates
        float yawRad = qDegreesToRadians(yaw);
        float pitchRad = qDegreesToRadians(pitch);

        // Calculate new position while maintaining distance
        cameraPosition = QVector3D(
                currentDistance * cos(pitchRad) * sin(yawRad),
                currentDistance * sin(pitchRad),
                currentDistance * cos(pitchRad) * cos(yawRad)
        );

        // Apply stored offset to both camera and target
        cameraPosition += currentOffset;
        cameraTarget = currentOffset;  // Target maintains offset from origin

        // Calculate camera orientation
        QVector3D forward = (cameraTarget - cameraPosition).normalized();
        QVector3D right = QVector3D::crossProduct(QVector3D(0, 1, 0), forward).normalized();
        cameraUp = QVector3D::crossProduct(forward, right);

        // Update view matrix
        view.setToIdentity();
        view.lookAt(cameraPosition, cameraTarget, cameraUp);

        // Debug output
        static int frameCount = 0;
        if (frameCount++ % 60 == 0) {
            qDebug() << "Camera State:"
                     << "\nPosition:" << cameraPosition
                     << "\nTarget:" << cameraTarget
                     << "\nDistance:" << currentDistance
                     << "\nOffset:" << currentOffset
                     << "\nYaw:" << yaw
                     << "\nPitch:" << pitch;
        }

        update();
    }

    void GLWidget::mouseMoveEvent(QMouseEvent* event) {
        if (!event->buttons()) return;

        QPoint delta = event->pos() - lastMousePos;

        if (isRotating) {
            // Update rotation angles
            yaw += delta.x() * rotationSpeed;
            pitch = qBound(-89.0f, pitch + delta.y() * rotationSpeed, 89.0f);
            updateCamera();
        } else if (isPanning) {
            // Calculate pan in camera space
            QVector3D forward = (cameraTarget - cameraPosition).normalized();
            QVector3D right = QVector3D::crossProduct(forward, cameraUp).normalized();
            QVector3D up = QVector3D::crossProduct(right, forward).normalized();

            QVector3D translation =
                    right * (-delta.x() * panSpeed) +
                    up * (delta.y() * panSpeed);

            // Apply translation to both camera and target
            cameraPosition += translation;
            cameraTarget += translation;

            // Update view matrix without changing orientation
            view.setToIdentity();
            view.lookAt(cameraPosition, cameraTarget, cameraUp);
            update();
        }

        lastMousePos = event->pos();
    }

    void GLWidget::wheelEvent(QWheelEvent* event) {
        // Get zoom direction
        float zoomFactor = event->angleDelta().y() / 120.0f;

        // Update camera position along view direction
        QVector3D viewDir = (cameraTarget - cameraPosition).normalized();
        cameraPosition += viewDir * zoomFactor * zoomSpeed;

        // Prevent getting too close or too far
        float distance = (cameraPosition - cameraTarget).length();
        if (distance < 1.0f) {
            cameraPosition = cameraTarget - viewDir;
        } else if (distance > 20.0f) {
            cameraPosition = cameraTarget - viewDir * 20.0f;
        }

        // Update view matrix without changing orientation
        view.setToIdentity();
        view.lookAt(cameraPosition, cameraTarget, cameraUp);
        update();
    }

    void GLWidget::resetCamera() {
        // Reset all camera parameters
        cameraPosition = QVector3D(0.0f, 0.0f, 5.0f);
        cameraTarget = QVector3D(0.0f, 0.0f, 0.0f);
        cameraUp = QVector3D(0.0f, 1.0f, 0.0f);
        pitch = 0.0f;
        yaw = 0.0f;

        view.setToIdentity();
        view.lookAt(cameraPosition, cameraTarget, cameraUp);
        update();
    }

    void GLWidget::mousePressEvent(QMouseEvent* event) {
        lastMousePos = event->pos();
        qDebug() << "Mouse pressed:" << event->button() << "at" << event->pos();

        if (event->button() == Qt::LeftButton) {
            isRotating = true;
            qDebug() << "Rotation started";
        } else if (event->button() == Qt::RightButton) {
            isPanning = true;
            qDebug() << "Panning started";
        }

        setCursor(Qt::ClosedHandCursor);
    }

    void GLWidget::mouseReleaseEvent(QMouseEvent* event) {
        if (event->button() == Qt::LeftButton) {
            isRotating = false;
        } else if (event->button() == Qt::RightButton) {
            isPanning = false;
        }

        // Restore cursor
        setCursor(Qt::ArrowCursor);
    }



    QVector3D GLWidget::getArcballVector(const QPoint& screenPos) {
        // Convert screen coordinates to normalized device coordinates (-1 to 1)
        float x = (2.0f * screenPos.x()) / width() - 1.0f;
        float y = 1.0f - (2.0f * screenPos.y()) / height();
        float z = 0.0f;
        float len2 = x * x + y * y;

        // If the point is inside the sphere
        if (len2 <= 1.0f) {
            z = std::sqrt(1.0f - len2);
        } else {
            // Normalize the vector for points outside the sphere
            float len = std::sqrt(len2);
            x /= len;
            y /= len;
        }

        return QVector3D(x, y, z);
    }

    void GLWidget::initializeGL() {
        initializeOpenGLFunctions();

        // Set up OpenGL state
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);  // Add explicit depth function

        // Face culling setup
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        // Enable multisampling if available
        glEnable(GL_MULTISAMPLE);

        // Print OpenGL debug info
        qDebug() << "OpenGL Version:" << QString((const char*)glGetString(GL_VERSION));
        qDebug() << "GLSL Version:" << QString((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

        setupShaders();
        setupBuffers();
        updateProjectionMatrix();
    }

    void GLWidget::resizeGL(int w, int h) {
        glViewport(0, 0, w, h);
        updateProjectionMatrix();
    }

    void GLWidget::updateProjectionMatrix() {
        float aspect = float(width()) / float(height() ? height() : 1);
        projection.setToIdentity();
        projection.perspective(45.0f, aspect, 0.1f, 100.0f);
    }

    void GLWidget::paintGL() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!program->isLinked()) {
            qDebug() << "Shader program not linked!";
            return;
        }

        program->bind();

        // Static rotation for testing
        static float angle = 0.0f;
        angle += 0.5f;
        QMatrix4x4 modelRotation;
        modelRotation.rotate(angle, QVector3D(0.0f, 1.0f, 0.0f));

        // Debug info
        static int frameCount = 0;
        if (frameCount++ % 60 == 0) {  // Print every 60 frames
            qDebug() << "Current rotation angle:" << angle;
            qDebug() << "Camera position:" << cameraPosition;
            qDebug() << "Depth test enabled:" << glIsEnabled(GL_DEPTH_TEST);
            qDebug() << "Face culling enabled:" << glIsEnabled(GL_CULL_FACE);
        }

        // Set matrices
        program->setUniformValue("projection", projection);
        program->setUniformValue("view", view);
        program->setUniformValue("model", modelRotation);

        // Draw cube
        vao.bind();
        glDrawArrays(GL_TRIANGLES, 0, 36);
        vao.release();

        // Draw axes and grid
        if (showAxes) {
            drawAxes();
        }
        if (showGrid) {
            drawGrid();
        }

        program->release();

        // Continue animation
        update();
    }

    void GLWidget::drawCamera() {
        QVector3D pos = cameraPosition;
        QVector3D target = cameraTarget;
        QVector3D up = cameraUp;

        // Draw camera frustum lines
        static const float lineVertices[] = {
                // Camera position to target
                pos.x(), pos.y(), pos.z(), 1.0f, 1.0f, 1.0f,
                target.x(), target.y(), target.z(), 1.0f, 1.0f, 1.0f,

                // Up vector
                pos.x(), pos.y(), pos.z(), 0.0f, 1.0f, 0.0f,
                pos.x() + up.x(), pos.y() + up.y(), pos.z() + up.z(), 0.0f, 1.0f, 0.0f
        };

        // Draw using the same shader
        QMatrix4x4 debugModel;
        program->setUniformValue("model", debugModel);

        QOpenGLBuffer debugVBO(QOpenGLBuffer::VertexBuffer);
        debugVBO.create();
        debugVBO.bind();
        debugVBO.allocate(lineVertices, sizeof(lineVertices));

        program->enableAttributeArray(0);
        program->setAttributeBuffer(0, GL_FLOAT, 0, 3, 6 * sizeof(float));
        program->enableAttributeArray(1);
        program->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 6 * sizeof(float));

        glDrawArrays(GL_LINES, 0, 4);
        debugVBO.destroy();
    }


    void GLWidget::updateOrientation(const Quaterniond& orientation) {
        // Convert Eigen quaternion to QMatrix4x4
        Eigen::Matrix3d rotMat = orientation.toRotationMatrix();
        QMatrix4x4 rotationMatrix;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                rotationMatrix(i, j) = rotMat(i, j);
            }
        }

        model = rotationMatrix;
        update(); // Request a redraw
    }

    void GLWidget::setShowAxes(bool show) {
        showAxes = show;
        update(); // Request a redraw
    }

    void GLWidget::setShowGrid(bool show) {
        showGrid = show;
        update(); // Request a redraw
    }

    void GLWidget::setupShaders() {
        const char* vertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec3 position;
    layout(location = 1) in vec3 color;

    uniform mat4 projection;
    uniform mat4 view;
    uniform mat4 model;

    out vec3 vertexColor;
    out vec3 fragPos;

    void main() {
        vertexColor = color;
        fragPos = vec3(model * vec4(position, 1.0));
        gl_Position = projection * view * model * vec4(position, 1.0);
    }
)";

        const char* fragmentShaderSource = R"(
    #version 330 core
    in vec3 vertexColor;
    in vec3 fragPos;
    out vec4 fragColor;

    void main() {
        vec3 lightPos = vec3(5.0, 5.0, 5.0);
        vec3 lightDir = normalize(lightPos - fragPos);
        float diff = max(dot(normalize(vec3(0.0, 1.0, 0.0)), lightDir), 0.0);
        vec3 diffuse = vertexColor * (0.5 + 0.5 * diff);  // Basic diffuse lighting
        fragColor = vec4(diffuse, 1.0);
    }
)";

        // Add error checking
        if (!program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
            qDebug() << "Vertex shader compilation failed:" << program->log();
            return;
        }

        if (!program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
            qDebug() << "Fragment shader compilation failed:" << program->log();
            return;
        }

        if (!program->link()) {
            qDebug() << "Shader program linking failed:" << program->log();
            return;
        }

        // Verify locations
        qDebug() << "position attribute location:" << program->attributeLocation("position");
        qDebug() << "color attribute location:" << program->attributeLocation("color");
    }

    void GLWidget::setupBuffers() {
        static const float vertices[] = {
                // Front face (red) - CCW winding
                -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  // Bottom left
                0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  // Bottom right
                0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  // Top right
                0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  // Top right
                -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  // Top left
                -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  // Bottom left

                // Back face (green) - CCW when looking from back
                -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
                0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
                0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
                0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
                -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
                -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,

                // Top face (blue)
                -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
                -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
                0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
                0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
                0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
                -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,

                // Bottom face (yellow)
                -0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
                0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
                0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
                0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
                -0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
                -0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,

                // Right face (magenta)
                0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
                0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
                0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
                0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
                0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
                0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,

                // Left face (cyan)
                -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
                -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
                -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
                -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
                -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
                -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f
        };


        // Main object VAO/VBO setup
            vao.create();
            vao.bind();

            vbo.create();
            vbo.bind();
            vbo.allocate(vertices, sizeof(vertices));

            // Set up vertex attributes
            program->enableAttributeArray(0);
            program->setAttributeBuffer(0, GL_FLOAT, 0, 3, 6 * sizeof(float));
            program->enableAttributeArray(1);
            program->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 6 * sizeof(float));

            vao.release();

        // Set up axes VAO/VBO
        axesVAO.create();
        axesVAO.bind();

        static const float axesVertices[] = {
                // x-axis (red)
                0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                // y-axis (green)
                0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                // z-axis (blue)
                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f
        };

        axesVBO.create();
        axesVBO.bind();
        axesVBO.allocate(axesVertices, sizeof(axesVertices));

        program->enableAttributeArray(0);
        program->setAttributeBuffer(0, GL_FLOAT, 0, 3, 6 * sizeof(float));
        program->enableAttributeArray(1);
        program->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 6 * sizeof(float));

        axesVAO.release();

        // Set up grid VAO/VBO
        gridVAO.create();
        gridVBO.create();
    }

    void GLWidget::drawAxes() {
        if (!axesVAO.isCreated()) return;

        QMatrix4x4 axesModel;
        axesModel.scale(2.0f); // Scale axes to make them more visible

        program->setUniformValue("model", axesModel);

        axesVAO.bind();
        glDrawArrays(GL_LINES, 0, 6);
        axesVAO.release();
    }

    void GLWidget::drawGrid() {
        if (!gridVAO.isCreated()) return;

        // Create grid points if not already created
        static std::vector<float> gridVertices;
        if (gridVertices.empty()) {
            const float gridSize = 5.0f;
            const float step = 0.5f;
            const float gridColor[3] = {0.5f, 0.5f, 0.5f};

            for (float x = -gridSize; x <= gridSize; x += step) {
                // Lines along Z
                gridVertices.push_back(x);
                gridVertices.push_back(0.0f);
                gridVertices.push_back(-gridSize);
                gridVertices.insert(gridVertices.end(), gridColor, gridColor + 3);

                gridVertices.push_back(x);
                gridVertices.push_back(0.0f);
                gridVertices.push_back(gridSize);
                gridVertices.insert(gridVertices.end(), gridColor, gridColor + 3);

                // Lines along X
                gridVertices.push_back(-gridSize);
                gridVertices.push_back(0.0f);
                gridVertices.push_back(x);
                gridVertices.insert(gridVertices.end(), gridColor, gridColor + 3);

                gridVertices.push_back(gridSize);
                gridVertices.push_back(0.0f);
                gridVertices.push_back(x);
                gridVertices.insert(gridVertices.end(), gridColor, gridColor + 3);
            }
        }

        gridVAO.bind();
        gridVBO.bind();
        gridVBO.allocate(gridVertices.data(), gridVertices.size() * sizeof(float));

        program->enableAttributeArray(0);
        program->setAttributeBuffer(0, GL_FLOAT, 0, 3, 6 * sizeof(float));
        program->enableAttributeArray(1);
        program->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 6 * sizeof(float));

        QMatrix4x4 gridModel;
        gridModel.translate(0.0f, -2.0f, 0.0f); // Place grid below the object
        program->setUniformValue("model", gridModel);

        glDrawArrays(GL_LINES, 0, gridVertices.size() / 6);
        gridVAO.release();
    }

}