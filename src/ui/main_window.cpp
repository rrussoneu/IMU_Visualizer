//
// Created by Raphael Russo on 11/20/24.
//

#include "main_window.h"
#include "transport/mock_transport.h"
#include <QDockWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

namespace imu_viz {

    MainWindow::MainWindow(QWidget* parent)
            : QMainWindow(parent)
            , transport(std::make_unique<MockTransport>())
            , glWidget(new GLWidget(this))
    {
        setCentralWidget(glWidget);
        setupUI();
        setupMenus();
        setupDockWidgets();

        // Connect transport signals
        transport->setDataCallback([this](const IMUData& data) {
            // QueuedConnection to ensure thread safety
            QMetaObject::invokeMethod(this, "handleIMUData",
                                      Qt::QueuedConnection, Q_ARG(IMUData, data));
        });

        transport->setErrorCallback([this](const std::string& error) {
            QMetaObject::invokeMethod(this, "handleError",
                                      Qt::QueuedConnection, Q_ARG(std::string, error));
        });

        statusBar()->showMessage("Ready");
    }

    void MainWindow::setupUI() {
        setWindowTitle("IMU Visualizer");
        resize(1024, 768);

        // Create toolbar
        auto toolbar = addToolBar("Main");

        auto connectButton = new QPushButton("Connect", this);
        toolbar->addWidget(connectButton);
        connect(connectButton, &QPushButton::clicked, this, &MainWindow::handleConnect);

        toolbar->addSeparator();

        auto showAxesButton = new QPushButton("Show Axes", this);
        showAxesButton->setCheckable(true);
        showAxesButton->setChecked(true);
        toolbar->addWidget(showAxesButton);
        connect(showAxesButton, &QPushButton::toggled, glWidget, &GLWidget::setShowAxes);

        auto showGridButton = new QPushButton("Show Grid", this);
        showGridButton->setCheckable(true);
        showGridButton->setChecked(true);
        toolbar->addWidget(showGridButton);
        connect(showGridButton, &QPushButton::toggled, glWidget, &GLWidget::setShowGrid);
    }

    void MainWindow::setupMenus() {
        auto fileMenu = menuBar()->addMenu("&File");

        auto connectAction = fileMenu->addAction("&Connect");
        connect(connectAction, &QAction::triggered, this, &MainWindow::handleConnect);

        fileMenu->addSeparator();

        auto exitAction = fileMenu->addAction("E&xit");
        connect(exitAction, &QAction::triggered, this, &QWidget::close);

        auto viewMenu = menuBar()->addMenu("&View");

        auto showAxesAction = viewMenu->addAction("Show &Axes");
        showAxesAction->setCheckable(true);
        showAxesAction->setChecked(true);
        connect(showAxesAction, &QAction::toggled, glWidget, &GLWidget::setShowAxes);

        auto showGridAction = viewMenu->addAction("Show &Grid");
        showGridAction->setCheckable(true);
        showGridAction->setChecked(true);
        connect(showGridAction, &QAction::toggled, glWidget, &GLWidget::setShowGrid);
    }

    void MainWindow::setupDockWidgets() {
        auto controlDock = new QDockWidget("Controls", this);
        controlDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

        auto controlWidget = new QWidget(controlDock);
        auto layout = new QVBoxLayout(controlWidget);

        // Control widgets
        auto calibrateButton = new QPushButton("Calibrate", controlWidget);
        layout->addWidget(calibrateButton);

        layout->addStretch();
        controlDock->setWidget(controlWidget);

        addDockWidget(Qt::RightDockWidgetArea, controlDock);
    }

    void MainWindow::handleConnect() {
        if (transport->isConnected()) {
            transport->disconnect();
            statusBar()->showMessage("Disconnected");
        } else {
            if (transport->connect()) {
                statusBar()->showMessage("Connected");
            } else {
                QMessageBox::warning(this, "Connection Error",
                                     "Failed to connect to the device.");
            }
        }
    }

    void MainWindow::handleIMUData(const IMUData& data) {
        // Convert acceleration to orientation
        // This is a simplified version - need to add proper sensor fusion
        Vector3d accel = data.acceleration.normalized();
        Vector3d up(0, 0, 1);

        Quaterniond orientation = Quaterniond::FromTwoVectors(up, accel);
        glWidget->updateOrientation(orientation);

        // Update status
        statusBar()->showMessage(QString("Last update: %1 ms").arg(data.timestamp / 1000.0));
    }

    void MainWindow::handleError(const std::string& error) {
        QMessageBox::warning(this, "Error", QString::fromStdString(error));
    }

}