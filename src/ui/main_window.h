//
// Created by Raphael Russo on 11/20/24.
//

#ifndef IMU_VISUALIZER_MAIN_WINDOW_H
#define IMU_VISUALIZER_MAIN_WINDOW_H

#pragma once
#include <QMainWindow>
#include <memory>
#include "visualization/gl_widget.h"
#include "transport/transport_interface.h"

namespace imu_viz {
    class MainWindow : public QMainWindow {
        Q_OBJECT
    public:
        explicit MainWindow(QWidget *parent = nullptr);

    private slots:
        void handleConnect();
        void handleIMUData(const IMUData& data);
        void handleError(const std::string& error);

    private:
        std::unique_ptr<ITransport> transport;
        GLWidget* glWidget;

        void setupUI();
        void setupMenus();
        void setupDockWidgets();

    };
}


#endif //IMU_VISUALIZER_MAIN_WINDOW_H
