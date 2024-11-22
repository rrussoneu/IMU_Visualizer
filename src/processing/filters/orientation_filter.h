//
// Created by Raphael Russo on 11/21/24.
//

#ifndef IMU_VISUALIZER_ORIENTATION_FILTER_H
#define IMU_VISUALIZER_ORIENTATION_FILTER_H
#pragma once
#include <Eigen/Geometry>
#include "imu_visualizer/common.h"

namespace imu_viz {

    class IOrientationFilter {
    public:
        virtual ~IOrientationFilter() = default;

        virtual void update(const Vector3d &accel, const Vector3d &gyro, double dt) = 0;

        virtual const Quaterniond &getOrientation() const = 0;

        virtual void reset() = 0;
    };
}
#endif //IMU_VISUALIZER_ORIENTATION_FILTER_H
