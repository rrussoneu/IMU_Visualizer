//
// Created by Raphael Russo on 11/21/24.
//

#ifndef IMU_VISUALIZER_SERIAL_TRANSPORT_H
#define IMU_VISUALIZER_SERIAL_TRANSPORT_H
#pragma once

#include "transport_interface.h"
#include "core/imu_data.h"
#include <QTimer>
#include <memory>
#include <QSerialPort>

namespace imu_viz {

    class SerialTransport : public QObject, public ITransport {
        Q_OBJECT
    public:
        explicit SerialTransport(QObject* parent = nullptr);
        ~SerialTransport() override;

        bool connect() override;
        bool disconnect() override;
        bool isConnected() const override;

        // Config
        void setPort(const QString& portName);
        void setBaudRate(qint32 baudRate);

        // Helper
        static QStringList availablePorts();

    private slots:
        void handleReadyRead();
        void handleError(QSerialPort::SerialPortError error);
        void handleTimeout();

    private:
        std::unique_ptr<QSerialPort> port;
        QTimer timeoutTimer;
        QByteArray buffer;

        // Config
        QString portName;
        qint32 baudRate{115200};

        /**
         * Packets
         * 0: start marker
         * 1-12: 3 floats from accelerometer
         * 13-24: 3 floats from gyroscope
         * 25: End marker
         */

        // Constants
        static constexpr uint8_t PACKET_START = 0xAA;
        static constexpr uint8_t PACKET_END = 0x55;
        static constexpr  size_t PACKET_SIZE = 26; // 24 bytes, 2 markers

        // Packet processing
        bool processPacket(const QByteArray& packet);
        uint8_t calculateChecksum(const uint8_t* data, size_t length) const;






    };
}



#endif //IMU_VISUALIZER_SERIAL_TRANSPORT_H
