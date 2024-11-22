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
#include <QGroupBox>
#include <QSlider>
#include <QFormLayout>
#include <QComboBox>
#include "transport/tcp_transport.h"
#include <QTimer>

namespace imu_viz {

    MainWindow::MainWindow(QWidget* parent)
            : QMainWindow(parent)
            , transport(std::make_unique<MockTransport>())
            , dataProcessor(new DataProcessor(this))
            , glWidget(new GLWidget(this))
    {
        setCentralWidget(glWidget);
        setupUI();
        setupMenus();
        setupDockWidgets();
        setupDataPipeline();


        statusBar()->showMessage("Ready");
    }

    void MainWindow::setupDataPipeline() {
        // Connect transport to data processor
        transport->setDataCallback([this](const IMUData& data) {
            // QueuedConnection for thread safety since transport runs in separate thread
            QMetaObject::invokeMethod(this->dataProcessor, "processIMUData",
                                      Qt::QueuedConnection,
                                      Q_ARG(IMUData, data));
        });

        transport->setErrorCallback([this](const std::string& error) {
            QMetaObject::invokeMethod(this, "handleError",
                                      Qt::QueuedConnection,
                                      Q_ARG(std::string, error));
        });

        // Connect data processor to GL widget
        connect(dataProcessor, &DataProcessor::newOrientation,
                glWidget, &GLWidget::updateOrientation);

        connect(dataProcessor, &DataProcessor::errorOccurred,
                this, [this](const QString& error) {
                    statusBar()->showMessage("Error: " + error, 3000);
                });

    }

    void MainWindow::setupUI() {
        setWindowTitle("IMU Visualizer");
        resize(1024, 768);

        // Create toolbar
        auto toolbar = addToolBar("Main");

        // Create transport controls group
        auto transportGroup = new QGroupBox("Connection", this);
        auto transportLayout = new QHBoxLayout(transportGroup);

        // Transport type selector
        auto transportCombo = new QComboBox(this);
        transportCombo->addItem("Mock Transport", QVariant::fromValue(TransportType::MOCK));
        transportCombo->addItem("TCP Transport", QVariant::fromValue(TransportType::TCP));
        transportLayout->addWidget(transportCombo);

        // Server info label
        auto infoLabel = new QLabel(this);
        infoLabel->setMinimumWidth(200);
        transportLayout->addWidget(infoLabel);

        // Connect button
        connectButton = new QPushButton("Connect", this);
        connectButton->setCheckable(true);
        transportLayout->addWidget(connectButton);

        // Connect transport selection handler
        connect(transportCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [=, this](int index) {  // Capture all by value and this pointer
                    if (connectButton->isChecked()) {
                        connectButton->click(); // Disconnect current transport
                    }

                    auto type = transportCombo->currentData().value<TransportType>();
                    switch (type) {
                        case TransportType::MOCK:
                            transport = std::make_unique<MockTransport>();
                            connectButton->setText("Connect");
                            infoLabel->clear();
                            break;
                        case TransportType::TCP:
                            transport = std::make_unique<TCPTransport>();
                            connectButton->setText("Start Server");
                            break;
                        default:
                            break;
                    }
                    setupDataPipeline();
                });

        // Connect button handler
        connect(connectButton, &QPushButton::toggled, this, [this, infoLabel](bool checked) {
            auto tcpTransport = dynamic_cast<TCPTransport*>(transport.get());

            if (checked) {
                if (transport->connect()) {
                    if (tcpTransport) {
                        connectButton->setText("Stop Server");
                        infoLabel->setText("Server IP: " + tcpTransport->getLocalAddress());
                    } else {
                        connectButton->setText("Disconnect");
                    }
                    statusBar()->showMessage("Connected");
                } else {
                    connectButton->setChecked(false);
                    QMessageBox::warning(this, "Connection Error",
                                         "Failed to connect to the device.");
                }
            } else {
                transport->disconnect();
                if (tcpTransport) {
                    connectButton->setText("Start Server");
                    infoLabel->clear();
                } else {
                    connectButton->setText("Connect");
                }
                statusBar()->showMessage("Disconnected");
            }
        });


        toolbar->addWidget(transportGroup);
        toolbar->addSeparator();

        // Calibration control
        auto calibrateButton = new QPushButton("Calibrate", this);
        calibrateButton->setCheckable(true);
        toolbar->addWidget(calibrateButton);
        connect(calibrateButton, &QPushButton::toggled, this, [this, calibrateButton](bool checked) {
            if (checked) {
                dataProcessor->startCalibration();
                calibrateButton->setText("Stop Calibration");
            } else {
                dataProcessor->finishCalibration();
                calibrateButton->setText("Calibrate");
            }
        });


        auto resetButton = new QPushButton("Reset Orientation", this);
        toolbar->addWidget(resetButton);
        connect(resetButton, &QPushButton::clicked, dataProcessor, &DataProcessor::resetOrientation);

        // Add visualization controls
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

        // Camera controls group
        auto cameraGroup = new QGroupBox("Camera", controlWidget);
        auto cameraLayout = new QVBoxLayout(cameraGroup);

        // Reset camera button
        auto resetCameraButton = new QPushButton("Reset Camera", cameraGroup);
        connect(resetCameraButton, &QPushButton::clicked, glWidget, &GLWidget::resetCamera);
        cameraLayout->addWidget(resetCameraButton);

        // Camera speed controls
        auto rotationSpeedSlider = new QSlider(Qt::Horizontal, cameraGroup);
        rotationSpeedSlider->setRange(1, 100);
        rotationSpeedSlider->setValue(50);
        connect(rotationSpeedSlider, &QSlider::valueChanged,
                [this](int value) { glWidget->setRotationSpeed(value / 100.0f); });

        auto speedLayout = new QFormLayout;
        speedLayout->addRow("Rotation Speed:", rotationSpeedSlider);
        cameraLayout->addLayout(speedLayout);

        layout->addWidget(cameraGroup);
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