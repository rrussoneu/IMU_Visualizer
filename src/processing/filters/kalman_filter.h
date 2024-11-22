//
// Created by Raphael Russo on 11/21/24.
//

#ifndef IMU_VISUALIZER_KALMAN_FILTER_H
#define IMU_VISUALIZER_KALMAN_FILTER_H
#include "orientation_filter.h"


namespace imu_viz {
    class KalmanFilter : public IOrientationFilter {
    public:
        KalmanFilter()
                : currentOrientation(Quaterniond::Identity())
        {
            initializeState();
        }

        void update(const Vector3d& accel, const Vector3d& gyro, double dt) override {
            predict(gyro, dt);
            correct(accel);
        }

        const Quaterniond& getOrientation() const override {
            return currentOrientation;
        }

        void reset() override {
            currentOrientation = Quaterniond::Identity();
            initializeState();
        }

    private:
        void initializeState() {
            // Initialize Kalman filter state matrices
            stateCovariance = Eigen::Matrix3d::Identity() * 0.1;
            processNoise = Eigen::Matrix3d::Identity() * 0.001;
            measurementNoise = Eigen::Matrix3d::Identity() * 0.1;
        }

        Eigen::Matrix3d skewSymmetric(const Vector3d& v) {
            Eigen::Matrix3d skew;
            skew <<      0,   -v(2),  v(1),
                    v(2),      0,  -v(0),
                    -v(1),   v(0),     0;
            return skew;
        }

        void predict(const Vector3d& gyro, double dt) {
            // Go back to CV notes if needed
            // Predict the new orientation based on gyro measurements
            // Update currentOrientation quaternion and state covariance matrix

            // Compute the angular displacement
            Vector3d delta_theta = gyro * dt;

            // Convert delta theta to a quaternion
            double angle = delta_theta.norm();
            Quaterniond delta_q;
            if (angle > 1e-6) {
                Vector3d axis = delta_theta / angle;
                delta_q = Quaterniond(Eigen::AngleAxisd(angle, axis));
            } else {
                delta_q = Quaterniond::Identity();
            }

            // Update current orientation
            currentOrientation = currentOrientation * delta_q;
            currentOrientation.normalize();

            // State transition matrix
            Eigen::Matrix3d F = Eigen::Matrix3d::Identity() - skewSymmetric(gyro * dt);

            // Predict state covariance
            stateCovariance = F * stateCovariance * F.transpose() + processNoise;
        }

        void correct(const Vector3d& accel) {
            // Correct the orientation estimate using accelerometer measurements

            // Normalize accelerometer measurement
            Vector3d z = accel.normalized();

            // Expected measurement based on current orientation
            Vector3d gravity_world(0, 0, -1);
            Vector3d h = currentOrientation.conjugate() * gravity_world;

            // Measurement residual
            Vector3d y = z - h;

            // Measurement matrix H
            Eigen::Matrix3d H = -skewSymmetric(h);

            // Kalman gain
            Eigen::Matrix3d S = H * stateCovariance * H.transpose() + measurementNoise;
            Eigen::Matrix3d K = stateCovariance * H.transpose() * S.inverse();

            // Update state estimate
            Vector3d delta_theta = K * y;

            // Update orientation
            // Convert delta theta to quaternion
            double angle = delta_theta.norm();
            Quaterniond delta_q;
            if (angle > 1e-6) {
                Vector3d axis = delta_theta / angle;
                delta_q = Quaterniond(Eigen::AngleAxisd(angle, axis));
            } else {
                delta_q = Quaterniond::Identity();
            }

            currentOrientation = delta_q * currentOrientation;
            currentOrientation.normalize();

            // Update state covariance
            Eigen::Matrix3d I = Eigen::Matrix3d::Identity();
            stateCovariance = (I - K * H) * stateCovariance;
        }

        Quaterniond currentOrientation;
        Eigen::Matrix3d stateCovariance;
        Eigen::Matrix3d processNoise;
        Eigen::Matrix3d measurementNoise;
    };
}
#endif //IMU_VISUALIZER_KALMAN_FILTER_H
