//
// Created by Raphael Russo on 11/22/24.
//

#ifndef IMU_VISUALIZER_TCP_TRANSPORT_H
#define IMU_VISUALIZER_TCP_TRANSPORT_H
#include "transport_interface.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QNetworkInterface>

namespace imu_viz {

    class TCPTransport : public QObject, public ITransport {
    Q_OBJECT

    public:
        explicit TCPTransport(QObject* parent = nullptr)
                : QObject(parent)
                , server(new QTcpServer(this))
                , clientSocket(nullptr)
                , port(8080)
        {
            // Setup server signal
            QObject::connect(server, &QTcpServer::newConnection,
                    this, &TCPTransport::handleNewConnection);
        }

        bool connect() override {
            if (server->isListening()) return true;

            if (!server->listen(QHostAddress::Any, port)) {
                if (errorCallback) {
                    errorCallback("Failed to start server: " +
                                  server->errorString().toStdString());
                }
                return false;
            }

            qDebug() << "Server listening on port" << port;
            return true;
        }

        bool disconnect() override {
            if (clientSocket) {
                clientSocket->disconnectFromHost();
                clientSocket->deleteLater();
                clientSocket = nullptr;
            }
            server->close();
            return true;
        }

        bool isConnected() const override {
            return clientSocket &&
                   clientSocket->state() == QAbstractSocket::ConnectedState;
        }

        void setPort(quint16 newPort) {
            if (!server->isListening()) {
                port = newPort;
            }
        }

        QString getLocalAddress() const {
            QString addresses;
            const QList<QHostAddress> ipAddressList = QNetworkInterface::allAddresses();
            for (const QHostAddress &address : ipAddressList) {
                if (address.protocol() == QAbstractSocket::IPv4Protocol &&
                    address != QHostAddress::LocalHost) {
                    addresses += address.toString() + " ";
                }
            }
            return addresses;
        }

    private slots:
        void handleNewConnection() {
            if (clientSocket) {
                // One connection at a time
                QTcpSocket* newSocket = server->nextPendingConnection();
                newSocket->disconnectFromHost();
                newSocket->deleteLater();
                return;
            }

            clientSocket = server->nextPendingConnection();
            if (!clientSocket) return;

            qDebug() << "New client connected from" <<
                     clientSocket->peerAddress().toString();

            QObject::connect(clientSocket, &QTcpSocket::disconnected,
                    this, &TCPTransport::handleClientDisconnected);
            QObject::connect(clientSocket, &QTcpSocket::errorOccurred,
                    this, &TCPTransport::handleError);
            QObject::connect(clientSocket, &QTcpSocket::readyRead,
                    this, &TCPTransport::handleReadyRead);

            buffer.clear();
        }

        void handleClientDisconnected() {
            qDebug() << "Client disconnected";
            if (clientSocket) {
                clientSocket->deleteLater();
                clientSocket = nullptr;
            }
        }

        void handleError(QAbstractSocket::SocketError error) {
            if (errorCallback) {
                QString errorMsg = clientSocket ?
                                   clientSocket->errorString() :
                                   server->errorString();
                errorCallback(errorMsg.toStdString());
            }
        }

        void handleReadyRead() {
            if (!clientSocket) return;

            buffer.append(clientSocket->readAll());

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

    private:
        static constexpr char PACKET_START = 0xAA;
        static constexpr char PACKET_END = 0x55;
        static constexpr int PACKET_SIZE = 26;  // 1 start + 24 data + 1 end

        QTcpServer* server;
        QTcpSocket* clientSocket;
        quint16 port;
        QByteArray buffer;

        bool processPacket(const QByteArray& packet) {
            // Verify packet markers
            if (packet[0] != PACKET_START || packet[PACKET_SIZE-1] != PACKET_END) {
                qDebug() << "Invalid packet markers:"
                         << QString("0x%1").arg(packet[0], 2, 16, QChar('0'))
                         << QString("0x%1").arg(packet[PACKET_SIZE-1], 2, 16, QChar('0'));
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
            data.acceleration = Vector3d(-accel[0], accel[1], accel[2]); // Invert x after testing

            // Extract gyroscope data
            float gyro[3];
            memcpy(gyro, packet.data() + 13, 12);
            data.gyroscope = Vector3d(-gyro[0], gyro[1], gyro[2]);

            qDebug() << "Received IMU data:"
                     << "Accel:" << data.acceleration.x() << data.acceleration.y() << data.acceleration.z()
                     << "Gyro:" << data.gyroscope.x() << data.gyroscope.y() << data.gyroscope.z();

            if (dataCallback) {
                dataCallback(data);
            }

            return true;
        }
    };

} // namespace imu_viz

#endif //IMU_VISUALIZER_TCP_TRANSPORT_H
