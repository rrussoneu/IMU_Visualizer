//
// Created by Raphael Russo on 11/20/24.
//

#ifndef IMU_VISUALIZER_TRANSPORT_INTERFACE_H
#define IMU_VISUALIZER_TRANSPORT_INTERFACE_H
#pragma once

#include <vector>
#include <functional>
#include "core/imu_data.h"

namespace imu_viz {
    class ITransport {
    public:
        virtual ~ITransport() = default;

        using DataCallback = std::function<void(const IMUData&)>;
        using ErrorCallback = std::function<void(const std::string&)>;

        virtual bool connect() = 0;
        virtual bool disconnect() = 0;
        virtual bool isConnected() const = 0;

        void setDataCallback(DataCallback cb) { dataCallback = std::move(cb); }
        void setErrorCallback(ErrorCallback cb) { errorCallback = std::move(cb); }

    protected:
        DataCallback dataCallback;
        ErrorCallback errorCallback;
    };
}

#endif //IMU_VISUALIZER_TRANSPORT_INTERFACE_H
