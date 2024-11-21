//
// Created by Raphael Russo on 11/21/24.
//

#include "serial_transport.h"
#include <QSerialPortInfo>

namespace imu_viz {
    SerialTransport::SerialTransport(QObject* parent)
            : QObject(parent)
            , port(std::make_unique<QSerialPort>())
    {
        // Setup timeout timer
        timeoutTimer.setSingleShot(true);
        timeoutTimer.setInterval(1000);  // 1 second timeout

        QObject::connect(&timeoutTimer, &QTimer::timeout,
                         this, &SerialTransport::handleTimeout);

        QObject::connect(port.get(), &QSerialPort::readyRead,
                         this, &SerialTransport::handleReadyRead);

        QObject::connect(port.get(), &QSerialPort::errorOccurred,
                         this, &SerialTransport::handleError);
    }

    SerialTransport::~SerialTransport() {
        if (port && port->isOpen()) {
            port->close();
        }
    }

    bool SerialTransport::connect() {
        if (port->isOpen()) return true;

        port->setPortName(portName);
        port->setBaudRate(baudRate);
        port->setDataBits(QSerialPort::Data8);
        port->setParity(QSerialPort::NoParity);
        port->setStopBits(QSerialPort::OneStop);
        port->setFlowControl(QSerialPort::NoFlowControl);

        if (!port->open(QIODevice::ReadWrite)) {
            if (errorCallback) {
                errorCallback("Failed to open serial port: " +
                              port->errorString().toStdString());
            }
            return false;
        }

        buffer.clear();
        timeoutTimer.start();
        return true;
    }

    bool SerialTransport::disconnect() {
        if (!port->isOpen()) return true;

        port->close();
        buffer.clear();
        timeoutTimer.stop();
        return true;
    }

    bool SerialTransport::isConnected() const {
        return port->isOpen();
    }

    void SerialTransport::handleReadyRead() {
        timeoutTimer.start();  // Reset timeout
        buffer.append(port->readAll());

        // Process complete packets
        while (buffer.size() >= PACKET_SIZE) {
            int start = buffer.indexOf(PACKET_START);
            if (start < 0) {
                buffer.clear();
                break;
            }

            if (start > 0) {
                buffer.remove(0, start);
            }

            if (buffer.size() >= PACKET_SIZE) {
                QByteArray packet = buffer.left(PACKET_SIZE);
                if (processPacket(packet)) {
                    buffer.remove(0, PACKET_SIZE);
                } else {
                    buffer.remove(0, 1);
                }
            }
        }
    }

    bool SerialTransport::processPacket(const QByteArray& packet) {
        // Verify packet markers
        if (packet[0] != PACKET_START || packet[PACKET_SIZE-1] != PACKET_END) {
            return false;
        }

        // Create IMU data struct
        IMUData data;
        data.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()
        ).count();

        // Extract accelerometer data
        float accel[3];
        memcpy(accel, packet.data() + 1, 12);
        data.acceleration = Vector3d(accel[0], accel[1], accel[2]);

        // Extract gyroscope data
        float gyro[3];
        memcpy(gyro, packet.data() + 13, 12);
        data.gyroscope = Vector3d(gyro[0], gyro[1], gyro[2]);

        if (dataCallback) {
            dataCallback(data);
        }

        return true;
    }

    void SerialTransport::handleError(QSerialPort::SerialPortError error) {
        if (error == QSerialPort::NoError) return;

        QString errorMsg = "Serial port error: ";
        switch (error) {
            case QSerialPort::DeviceNotFoundError:
                errorMsg += "Device not found";
                break;
            case QSerialPort::PermissionError:
                errorMsg += "Permission denied";
                break;
            case QSerialPort::OpenError:
                errorMsg += "Failed to open device";
                break;
            case QSerialPort::TimeoutError:
                errorMsg += "Operation timed out";
                break;
            default:
                errorMsg += "Unknown error";
        }

        if (errorCallback) {
            errorCallback(errorMsg.toStdString());
        }
    }

    void SerialTransport::handleTimeout() {
        if (errorCallback) {
            errorCallback("Serial communication timeout");
        }
    }

    QStringList SerialTransport::availablePorts() {
        QStringList ports;
        for (const QSerialPortInfo& info : QSerialPortInfo::availablePorts()) {
            ports << info.portName();
        }
        return ports;
    }

}