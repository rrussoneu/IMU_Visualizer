//
// Created by Raphael Russo on 11/20/24.
//

#ifndef IMU_VISUALIZER_MOCK_TRANSPORT_H
#define IMU_VISUALIZER_MOCK_TRANSPORT_H
#pragma once

#include "transport_interface.h"
#include <thread>
#include <atomic>

namespace imu_viz {
    class MockTransport : public ITransport {
    public:
        MockTransport();
        ~MockTransport();

        bool connect() override;
        bool disconnect() override;
        bool isConnected() const override;

    private:
        std::atomic<bool> running{false};
        std::thread mockThread;
        void mockDataLoop();
    };
}



#endif //IMU_VISUALIZER_MOCK_TRANSPORT_H
