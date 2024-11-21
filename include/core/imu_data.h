//
// Created by Raphael Russo on 11/20/24.
//

#ifndef IMU_VISUALIZER_IMU_DATA_H
#define IMU_VISUALIZER_IMU_DATA_H
#pragma once

#include "imu_visualizer/common.h"

namespace imu_viz {
    struct IMUData {
        uint64_t timestamp;
        Vector3d acceleration;
        Vector3d gyroscope;
    };

    struct CalibrationData {
        // Accelerometer
        Eigen::Vector3d accelBias{0, 0, 0};
        Eigen::Matrix3d accelScale{Eigen::Matrix3d::Identity()};

        // Gyroscope
        Eigen::Vector3d gyroBias{0, 0, 0};
        Eigen::Matrix3d gyroScale{Eigen::Matrix3d::Identity()};

        // Save/load calibration
        bool saveToFile(const std::string& filename) const;
        bool loadFromFile(const std::string& filename);
    };
}
#endif //IMU_VISUALIZER_IMU_DATA_H
