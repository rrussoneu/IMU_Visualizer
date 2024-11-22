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

        // Reset cursor
        setCursor(Qt::ArrowCursor);
    }

    // Originally to be like threejs arcball camera, but not using for now
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
        glFrontFace(GL_CCW);  // CC winding
        glCullFace(GL_BACK);   // Cull back faces
        glEnable(GL_CULL_FACE);

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

        static bool firstFrame = true;
        if (firstFrame) {
            qDebug() << "OpenGL State:";
            qDebug() << "Depth test enabled:" << glIsEnabled(GL_DEPTH_TEST);
            qDebug() << "Face culling enabled:" << glIsEnabled(GL_CULL_FACE);
            qDebug() << "Main program linked:" << program->isLinked();
            qDebug() << "Simple program linked:" << simpleProgram->isLinked();
            qDebug() << "VAO created:" << vao.isCreated();
            qDebug() << "Axes VAO created:" << axesVAO.isCreated();
            qDebug() << "Grid VAO created:" << gridVAO.isCreated();
            firstFrame = false;
        }

        // Draw cube
        if (program->isLinked()) {
            program->bind();

            /*
            static float angle = 0.0f;
            angle += 0.5f;
            QMatrix4x4 modelRotation;
            modelRotation.rotate(angle, QVector3D(0.0f, 1.0f, 0.0f));
            QMatrix3x3 normalMatrix = modelRotation.normalMatrix();

            program->setUniformValue("projection", projection);
            program->setUniformValue("view", view);
            program->setUniformValue("model", modelRotation);
            program->setUniformValue("normalMatrix", normalMatrix);
            program->setUniformValue("viewPos", cameraPosition);
            program->setUniformValue("lightPos", QVector3D(5.0f, 0.0f, 5.0f));

            vao.bind();
            glDrawArrays(GL_TRIANGLES, 0, 36);
            vao.release();

            program->release();
             */
            QMatrix4x4 modelMatrix = model;  // Use the model matrix directly

            program->setUniformValue("projection", projection);
            program->setUniformValue("view", view);
            program->setUniformValue("model", modelMatrix);
            program->setUniformValue("normalMatrix", modelMatrix.normalMatrix());
            program->setUniformValue("viewPos", cameraPosition);
            program->setUniformValue("lightPos", QVector3D(5.0f, 0.0f, 5.0f));

            vao.bind();
            glDrawArrays(GL_TRIANGLES, 0, 36);
            vao.release();

            program->release();
        } else {
            qDebug() << "Main program not linked!";
        }

        // Draw axes and grid
        if (showAxes) drawAxes();
        if (showGrid) drawGrid();

        // Check for OpenGL errors
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            qDebug() << "OpenGL error:" << err;
        }

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
        // Convert Eigen quaternion to QMatrix
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
        update(); // Redraw
    }

    void GLWidget::setShowGrid(bool show) {
        showGrid = show;
        update(); // Redraw
    }

    void GLWidget::setupShaders() {
        // Desktop for mac testing vertex shader
        const char* vertexShaderDesktop = R"(
    #version 330 core
    layout(location = 0) in vec3 position;
    layout(location = 1) in vec3 color;
    layout(location = 2) in vec3 normal;

    uniform mat4 projection;
    uniform mat4 view;
    uniform mat4 model;
    uniform mat3 normalMatrix;

    out vec3 fragPos;
    out vec3 vertexColor;
    out vec3 fragNormal;

    void main() {
        vertexColor = color;
        fragPos = vec3(model * vec4(position, 1.0));

        // Transform normal to world space maintaining correct orientation
        fragNormal = normalize(normalMatrix * normal);

        gl_Position = projection * view * model * vec4(position, 1.0);
    }
)";

        const char* fragmentShaderDesktop = R"(
    #version 330 core
    in vec3 fragPos;
    in vec3 vertexColor;
    in vec3 fragNormal;

    out vec4 fragColor;

    uniform vec3 lightPos;    // Main light
    uniform vec3 viewPos;

    // Material properties
    const float ambientStrength = 0.15;
    const float diffuseStrength = 0.7;
    const float specularStrength = 0.8;
    const float shininess = 64.0;

    // Secondary light sources for better illumination
    const vec3 fillLightPos = vec3(-5.0, 3.0, -5.0);
    const vec3 fillLightColor = vec3(0.2, 0.2, 0.3);
    const float fillLightIntensity = 0.3;

    const vec3 rimLightDir = vec3(0.0, 0.0, -1.0);
    const vec3 rimLightColor = vec3(0.1, 0.1, 0.15);
    const float rimLightIntensity = 0.2;

    vec3 calculateLight(vec3 lightPosition, vec3 lightColor, float intensity) {
        vec3 normal = normalize(fragNormal);
        vec3 lightDir = normalize(lightPosition - fragPos);
        vec3 viewDir = normalize(viewPos - fragPos);
        vec3 halfwayDir = normalize(lightDir + viewDir);

        // Ambient
        vec3 ambient = ambientStrength * lightColor;

        // Diffuse
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diffuseStrength * diff * lightColor;

        // Specular (Blinn-Phong)
        float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
        vec3 specular = specularStrength * spec * lightColor;

        // Edge highlighting (rim lighting)
        float rim = 1.0 - max(dot(viewDir, normal), 0.0);
        rim = smoothstep(0.6, 1.0, rim);

        return intensity * (ambient + diffuse + specular);
    }

    void main() {
        // Main light (warm white)
        vec3 mainLight = calculateLight(lightPos, vec3(1.0, 0.95, 0.8), 1.0);

        // Fill light (cool blue)
        vec3 fillLight = calculateLight(fillLightPos, fillLightColor, fillLightIntensity);

        // Rim light
        vec3 normal = normalize(fragNormal);
        vec3 viewDir = normalize(viewPos - fragPos);
        float rim = 1.0 - max(dot(viewDir, normal), 0.0);
        rim = smoothstep(0.6, 1.0, rim);
        vec3 rimLight = rim * rimLightColor * rimLightIntensity;

        // Combine all lighting
        vec3 result = (mainLight + fillLight + rimLight) * vertexColor;

        // Tone mapping and gamma correction
        result = result / (result + vec3(1.0));  // HDR tone mapping
        result = pow(result, vec3(1.0/2.2));     // Gamma correction

        fragColor = vec4(result, 1.0);
    }
)";

        // Raspberry Pi (using OpenGL ES) vertex shader
        const char* vertexShaderES = R"(
        #version 100
        attribute vec3 position;
        attribute vec3 color;
        attribute vec3 normal;

        uniform mat4 projection;
        uniform mat4 view;
        uniform mat4 model;
        uniform mat3 normalMatrix;

        varying vec3 fragPos;
        varying vec3 vertexColor;
        varying vec3 fragNormal;

        void main() {
            vertexColor = color;
            fragPos = vec3(model * vec4(position, 1.0));
            // Transform normal to world space
            fragNormal = normalMatrix * normal;
            gl_Position = projection * view * model * vec4(position, 1.0);
        }
    )";

        const char* fragmentShaderES = R"(
        precision mediump float;
        varying vec3 fragPos;
        varying vec3 vertexColor;
        varying vec3 fragNormal;

        uniform vec3 lightPos;
        uniform vec3 viewPos;

        void main() {
            // Ambient
            float ambientStrength = 0.1;
            vec3 ambient = ambientStrength * vertexColor;

            // Diffuse
            vec3 norm = normalize(fragNormal);
            vec3 lightDir = normalize(lightPos - fragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * vertexColor;

            // Specular
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - fragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
            vec3 specular = specularStrength * spec * vec3(1.0);

            vec3 result = ambient + diffuse + specular;
            gl_FragColor = vec4(result, 1.0);
        }
    )";

        // Choose shader version based on platform
        const char* vertexSource;
        const char* fragmentSource;

#ifdef __APPLE__
        vertexSource = vertexShaderDesktop;
        fragmentSource = fragmentShaderDesktop;
        qDebug() << "Using Desktop OpenGL shaders";
#else
        vertexSource = vertexShaderES;
        fragmentSource = fragmentShaderES;
        qDebug() << "Using OpenGL ES shaders";
#endif

        // Compile shaders with error checking
        if (!program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexSource)) {
            qDebug() << "Vertex shader compilation failed:" << program->log();
            return;
        }

        if (!program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSource)) {
            qDebug() << "Fragment shader compilation failed:" << program->log();
            return;
        }

        if (!program->link()) {
            qDebug() << "Shader program linking failed:" << program->log();
            return;
        }

        // Set up lighting uniforms
        program->bind();
        program->setUniformValue("lightPos", QVector3D(5.0f, 5.0f, 5.0f));
        program->setUniformValue("viewPos", cameraPosition);
        program->setUniformValue("ambientColor", QVector3D(0.1f, 0.1f, 0.1f));
        program->setUniformValue("specularStrength", 0.5f);
        program->setUniformValue("shininess", 32.0f);
        program->release();


        simpleProgram = std::make_unique<QOpenGLShaderProgram>();

        // Simple shader for mac
        const char* simpleVertexShaderDesktop = R"(
            #version 330 core
            layout(location = 0) in vec3 position;
            layout(location = 1) in vec3 color;

            uniform mat4 projection;
            uniform mat4 view;
            uniform mat4 model;

            out vec3 vertexColor;

            void main() {
                vertexColor = color;
                gl_Position = projection * view * model * vec4(position, 1.0);
            }
        )";

        const char* simpleFragmentShaderDesktop = R"(
            #version 330 core
            in vec3 vertexColor;
            out vec4 fragColor;

            void main() {
                fragColor = vec4(vertexColor, 1.0);
            }
        )";

        // Simple shader for Pi
        const char* simpleVertexShaderES = R"(
            #version 100
            attribute vec3 position;
            attribute vec3 color;

            uniform mat4 projection;
            uniform mat4 view;
            uniform mat4 model;

            varying vec3 vertexColor;

            void main() {
                vertexColor = color;
                gl_Position = projection * view * model * vec4(position, 1.0);
            }
        )";

        const char* simpleFragmentShaderES = R"(
            precision mediump float;
            varying vec3 vertexColor;

            void main() {
                gl_FragColor = vec4(vertexColor, 1.0);
            }
        )";

        // Choose shader source based on platform
        const char* simpleVertexSource;
        const char* simpleFragmentSource;

#ifdef __APPLE__
        simpleVertexSource = simpleVertexShaderDesktop;
        simpleFragmentSource = simpleFragmentShaderDesktop;
        qDebug() << "Using Desktop OpenGL shaders for simpleProgram";
#else
        simpleVertexSource = simpleVertexShaderES;
        simpleFragmentSource = simpleFragmentShaderES;
        qDebug() << "Using OpenGL ES shaders for simpleProgram";
#endif

        if (!simpleProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, simpleVertexSource)) {
            qDebug() << "Simple vertex shader compilation failed:" << simpleProgram->log();
            return;
        }

        if (!simpleProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, simpleFragmentSource)) {
            qDebug() << "Simple fragment shader compilation failed:" << simpleProgram->log();
            return;
        }

        if (!simpleProgram->link()) {
            qDebug() << "Simple shader program linking failed:" << simpleProgram->log();
            return;
        }

    }

    void GLWidget::setupBuffers() {
        vao.create();
        vao.bind();
        static const float vertices[] = {
                // Positions, colors, normals
                // Front face (red)
                // First triangle
                -0.5f, -0.5f, +0.5f,  1.0f, 0.0f, 0.0f,   0.0f,  0.0f, +1.0f,  // Bottom-left
                +0.5f, -0.5f, +0.5f,  1.0f, 0.0f, 0.0f,   0.0f,  0.0f, +1.0f,  // Bottom-right
                +0.5f, +0.5f, +0.5f,  1.0f, 0.0f, 0.0f,   0.0f,  0.0f, +1.0f,  // Top-right
                // Second triangle
                -0.5f, -0.5f, +0.5f,  1.0f, 0.0f, 0.0f,   0.0f,  0.0f, +1.0f,  // Bottom-left
                +0.5f, +0.5f, +0.5f,  1.0f, 0.0f, 0.0f,   0.0f,  0.0f, +1.0f,  // Top-right
                -0.5f, +0.5f, +0.5f,  1.0f, 0.0f, 0.0f,   0.0f,  0.0f, +1.0f,  // Top-left

                // Back face (green)
                // First triangle
                -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f, -1.0f,  // Bottom-left
                +0.5f, +0.5f, -0.5f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f, -1.0f,  // Top-right
                +0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f, -1.0f,  // Bottom-right
                // Second triangle
                -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f, -1.0f,  // Bottom-left
                -0.5f, +0.5f, -0.5f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f, -1.0f,  // Top-left
                +0.5f, +0.5f, -0.5f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f, -1.0f,  // Top-right

                // Left face (blue)
                // First triangle
                -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,  -1.0f,  0.0f,  0.0f,  // Back-bottom
                -0.5f, +0.5f, +0.5f,  0.0f, 0.0f, 1.0f,  -1.0f,  0.0f,  0.0f,  // Front-top
                -0.5f, +0.5f, -0.5f,  0.0f, 0.0f, 1.0f,  -1.0f,  0.0f,  0.0f,  // Back-top
                // Second triangle
                -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,  -1.0f,  0.0f,  0.0f,  // Back-bottom
                -0.5f, -0.5f, +0.5f,  0.0f, 0.0f, 1.0f,  -1.0f,  0.0f,  0.0f,  // Front-bottom
                -0.5f, +0.5f, +0.5f,  0.0f, 0.0f, 1.0f,  -1.0f,  0.0f,  0.0f,  // Front-top

                // Right face (yellow)
                // First triangle
                +0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,  +1.0f,  0.0f,  0.0f,  // Back-bottom
                +0.5f, +0.5f, -0.5f,  1.0f, 1.0f, 0.0f,  +1.0f,  0.0f,  0.0f,  // Back-top
                +0.5f, +0.5f, +0.5f,  1.0f, 1.0f, 0.0f,  +1.0f,  0.0f,  0.0f,  // Front-top
                // Second triangle
                +0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,  +1.0f,  0.0f,  0.0f,  // Back-bottom
                +0.5f, +0.5f, +0.5f,  1.0f, 1.0f, 0.0f,  +1.0f,  0.0f,  0.0f,  // Front-top
                +0.5f, -0.5f, +0.5f,  1.0f, 1.0f, 0.0f,  +1.0f,  0.0f,  0.0f,  // Front-bottom

                // Top face (magenta)
                // First triangle
                -0.5f, +0.5f, -0.5f,  1.0f, 0.0f, 1.0f,   0.0f, +1.0f,  0.0f,  // Back-left
                +0.5f, +0.5f, +0.5f,  1.0f, 0.0f, 1.0f,   0.0f, +1.0f,  0.0f,  // Front-right
                +0.5f, +0.5f, -0.5f,  1.0f, 0.0f, 1.0f,   0.0f, +1.0f,  0.0f,  // Back-right
                // Second triangle
                -0.5f, +0.5f, -0.5f,  1.0f, 0.0f, 1.0f,   0.0f, +1.0f,  0.0f,  // Back-left
                -0.5f, +0.5f, +0.5f,  1.0f, 0.0f, 1.0f,   0.0f, +1.0f,  0.0f,  // Front-left
                +0.5f, +0.5f, +0.5f,  1.0f, 0.0f, 1.0f,   0.0f, +1.0f,  0.0f,  // Front-right

                // Bottom face (cyan)
                // First triangle
                -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,   0.0f, -1.0f,  0.0f,  // Back-left
                +0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,   0.0f, -1.0f,  0.0f,  // Back-right
                +0.5f, -0.5f, +0.5f,  0.0f, 1.0f, 1.0f,   0.0f, -1.0f,  0.0f,  // Front-right
                // Second triangle
                -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,   0.0f, -1.0f,  0.0f,  // Back-left
                +0.5f, -0.5f, +0.5f,  0.0f, 1.0f, 1.0f,   0.0f, -1.0f,  0.0f,  // Front-right
                -0.5f, -0.5f, +0.5f,  0.0f, 1.0f, 1.0f,   0.0f, -1.0f,  0.0f,  // Front-left
        };


        vbo.create();
        vbo.bind();
        vbo.allocate(vertices, sizeof(vertices));

        program->bind();

        // Position attribute
        program->enableAttributeArray(0);
        program->setAttributeBuffer(0, GL_FLOAT, 0, 3, 9 * sizeof(float));
        // Color attribute
        program->enableAttributeArray(1);
        program->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 9 * sizeof(float));
        // Normal attribute
        program->enableAttributeArray(2);
        program->setAttributeBuffer(2, GL_FLOAT, 6 * sizeof(float), 3, 9 * sizeof(float));

        program->release();
        vao.release();
        vbo.release();

        // Set up axes vertex objects
        axesVAO.create();
        axesVBO.create();

        axesVAO.bind();
        axesVBO.bind();

        static const float axesVertices[] = {
                // x (red)
                0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                // y (green)
                0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                // z (blue)
                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f
        };

        axesVBO.allocate(axesVertices, sizeof(axesVertices));

        simpleProgram->bind();  // Use simple program for axes <---
        simpleProgram->enableAttributeArray(0);
        simpleProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, 6 * sizeof(float));
        simpleProgram->enableAttributeArray(1);
        simpleProgram->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 6 * sizeof(float));
        simpleProgram->release();

        axesVAO.release();
        axesVBO.release();

        // Set up grid vertex objects
        gridVAO.create();
        gridVBO.create();
    }

    void GLWidget::drawAxes() {
        if (!axesVAO.isCreated()) return;

        simpleProgram->bind();

        QMatrix4x4 axesModel;
        axesModel.scale(2.0f); // Scale axes to make them more visible

        simpleProgram->setUniformValue("projection", projection);
        simpleProgram->setUniformValue("view", view);
        simpleProgram->setUniformValue("model", axesModel);

        axesVAO.bind();
        glDrawArrays(GL_LINES, 0, 6);
        axesVAO.release();

        simpleProgram->release();
    }

    void GLWidget::drawGrid() {
        if (!gridVAO.isCreated()) return;

        simpleProgram->bind();

        // Create grid points
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

            gridVBO.bind();
            gridVBO.allocate(gridVertices.data(), gridVertices.size() * sizeof(float));
            gridVBO.release();
        }

        gridVAO.bind();
        gridVBO.bind();

        simpleProgram->enableAttributeArray(0);
        simpleProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, 6 * sizeof(float));
        simpleProgram->enableAttributeArray(1);
        simpleProgram->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 6 * sizeof(float));

        QMatrix4x4 gridModel;
        gridModel.translate(0.0f, -2.0f, 0.0f); // Place grid below the object
        simpleProgram->setUniformValue("projection", projection);
        simpleProgram->setUniformValue("view", view);
        simpleProgram->setUniformValue("model", gridModel);

        glDrawArrays(GL_LINES, 0, gridVertices.size() / 6);
        gridVAO.release();

        simpleProgram->release();
    }

}
