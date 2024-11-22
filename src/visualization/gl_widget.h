//
// Created by Raphael Russo on 11/20/24.
//

#ifndef IMU_VISUALIZER_GL_WIDGET_H
#define IMU_VISUALIZER_GL_WIDGET_H
#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include "imu_visualizer/common.h"

namespace imu_viz {
    class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

    public:
        explicit GLWidget(QWidget* parent = nullptr);
        ~GLWidget();  // Add destructor to clean up OpenGL resources

        //void updateOrientation(const Quaterniond& orientation);
        void setShowAxes(bool show);
        void setShowGrid(bool show);
        void resetCamera();
        void setRotationSpeed(float speed) {
            rotationSpeed = speed;
            qDebug() << "Rotation speed set to:" << speed;
        }
        void setPanSpeed(float speed) {
            panSpeed = speed;
            qDebug() << "Pan speed set to:" << speed;
        }
        void setZoomSpeed(float speed) {
            zoomSpeed = speed;
            qDebug() << "Zoom speed set to:" << speed;
        }
    public slots:
        void updateOrientation(const imu_viz::Quaterniond& orientation);
    signals:
        void cameraChanged();

    protected:
        void initializeGL() override;
        void paintGL() override;
        void resizeGL(int w, int h) override;

        // Mouse event handlers
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;

    private:

        // Camera
        void drawCamera();
        void updateProjectionMatrix();
        QVector3D cameraPosition{0.0f, 0.0f, 5.0f};
        QVector3D cameraTarget{0.0f, 0.0f, 0.0f};
        QVector3D cameraUp{0.0f, 1.0f, 0.0f};

        // Mouse tracking
        QPoint lastMousePos;
        bool isRotating{false};
        bool isPanning{false};
        float rotationSpeed{0.5f};
        float panSpeed{0.01f};
        float zoomSpeed{0.1f};

        // Camera angles (in degrees)
        float pitch{0.0f};
        float yaw{0.0f};

        // Helper functions, act like the arcball camera from three js
        void updateCamera();
        QVector3D getArcballVector(const QPoint& screenPos);

        // Shader program
        std::unique_ptr<QOpenGLShaderProgram> program;
        std::unique_ptr<QOpenGLShaderProgram> simpleProgram;

        // Matrices
        QMatrix4x4 projection;
        QMatrix4x4 view;
        QMatrix4x4 model;

        // Vertex buffers
        QOpenGLVertexArrayObject vao;
        QOpenGLBuffer vbo;

        // Display flags
        bool showAxes{true};
        bool showGrid{true};

        // Additional buffers for axes and grid
        QOpenGLBuffer axesVBO;
        QOpenGLVertexArrayObject axesVAO;
        QOpenGLBuffer gridVBO;
        QOpenGLVertexArrayObject gridVAO;

        // Setup functions
        void setupShaders();
        void setupBuffers();
        void drawAxes();
        void drawGrid();
    };
}

#endif //IMU_VISUALIZER_GL_WIDGET_H
