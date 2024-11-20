//
// Created by Raphael Russo on 11/20/24.
//

#ifndef IMU_VISUALIZER_GL_WIDGET_H
#define IMU_VISUALIZER_GL_WIDGET_H
#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include "imu_visualizer/common.h"

namespace imu_viz {
class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit GLWidget(QWidget* parent = nullptr);

public slots:
    void updateOrientation(const Quaterniond &orientation);
    void setShowAxes(bool show);
    void setShowGrid(bool show);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    QOpenGLShaderProgram program;
    QMatrix4x4 projection;
    QMatrix4x4 view;
    QMatrix4x4 model;

    bool showAxes{true};
    bool showGrid{true};

    void setupShaders();
    void setupBuffers();
    void drawAxes();
    void drawGrid();

};
}


#endif //IMU_VISUALIZER_GL_WIDGET_H
