//
// Created by Raphael Russo on 11/20/24.
//

#include "mock_transport.h"

namespace imu_viz {
    MockTransport::MockTransport() = default;
    MockTransport::~MockTransport() {
        disconnect();
    }

    bool MockTransport::connect() {
        if (running) return true;

        running = true;
        mockThread = std::thread(&MockTransport::mockDataLoop, this);
        return true;
    }

    bool MockTransport::disconnect() {
        if (!running) return  true;

        running = false;
        if (mockThread.joinable()) {
            mockThread.join();
        }

        return true;
    }

    bool MockTransport::isConnected() const {
        return running;
    }

    void MockTransport::mockDataLoop() {
        using namespace std::chrono;
        auto start = high_resolution_clock ::now();

        while (running) {
            auto now = high_resolution_clock ::now();
            auto timestamp = duration_cast<microseconds>(now - start).count();

            IMUData data;
            data.timestamp = timestamp;

            double t = timestamp * 1e-6;
            data.acceleration = Vector3d(
                    std::sin(t),
                    std::cos(t),
                    9.81
                    );

            data.gyroscope = Vector3d(0,0,1);

            if (dataCallback) {
                dataCallback(data);
            }

            std::this_thread::sleep_for(milliseconds(10));
        }
    }
}