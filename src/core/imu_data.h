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

        bool calibrate{false};
        Vector3d accelBias{0,0,0};
        Vector3d gyroBias{0,0,0};
    };
}
#endif //IMU_VISUALIZER_IMU_DATA_H
