//
// Created by Raphael Russo on 11/21/24.
//

#ifndef IMU_VISUALIZER_COMPLEMENTARY_FILTER_H
#define IMU_VISUALIZER_COMPLEMENTARY_FILTER_H
#include "orientation_filter.h"
#include <Eigen/Geometry>
namespace imu_viz {
    class ComplementaryFilter : public IOrientationFilter {
    public:
        ComplementaryFilter(double accelWeight = 0.02)
                : accelweight(accelWeight), currentOrientation(Quaterniond::Identity()) {}

        void update(const Vector3d &accel, const Vector3d &gyro, double dt) override {
            // Gyroscope integration
            Vector3d gyroRad = gyro * dt;
            double angle = gyroRad.norm();
            Quaterniond gyroQuat;

            if (angle > 1e-10) {
                Vector3d axis = gyroRad / angle;
                gyroQuat = Quaterniond(Eigen::AngleAxisd(angle, axis));
            } else {
                gyroQuat = Quaterniond::Identity();
            }

            Quaterniond gyroOrientation = currentOrientation * gyroQuat;

            // Accelerometer orientation
            Vector3d accelNorm = accel.normalized();
            Quaterniond accelQuat = Quaterniond::FromTwoVectors(Vector3d::UnitZ(), accelNorm);

            // Complementary filter
            currentOrientation = gyroOrientation.slerp(accelweight, accelQuat);
            currentOrientation.normalize();
        }

        const Quaterniond &getOrientation() const override {
            return currentOrientation;
        }

        void reset() override {
            currentOrientation = Quaterniond::Identity();
        }

    private:
        double accelweight;
        Quaterniond currentOrientation;
    };
}
#endif //IMUVISUALIZER_COMPLEMENTARY_FILTER_H