//
// Created by Raphael Russo on 11/20/24.
//

#ifndef IMU_VISUALIZER_COMMON_H
#define IMU_VISUALIZER_COMMON_H

#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <QMetaType>

namespace imu_viz {
    using Vector3d = Eigen::Vector3d;
    using Quaterniond = Eigen::Quaterniond;
    using Matrix3d = Eigen::Matrix3d;
    using Matrix4d = Eigen::Matrix4d;
}

Q_DECLARE_METATYPE(imu_viz::Vector3d)
Q_DECLARE_METATYPE(imu_viz::Quaterniond)
Q_DECLARE_METATYPE(imu_viz::Matrix3d)

#endif //IMU_VISUALIZER_COMMON_H
