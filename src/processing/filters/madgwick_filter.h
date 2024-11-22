//
// Created by Raphael Russo on 11/21/24.
//

#ifndef IMU_VISUALIZER_MADGWICK_FILTER_H
#define IMU_VISUALIZER_MADGWICK_FILTER_H
#include "orientation_filter.h"
namespace imu_viz {
    class MadgwickFilter : public IOrientationFilter {
    public:
        MadgwickFilter(double beta = 0.1)
                : beta(beta), currentOrientation(Quaterniond::Identity()) {}

        void update(const Vector3d &accel, const Vector3d &gyro, double dt) override {
            double q0 = currentOrientation.w();
            double q1 = currentOrientation.x();
            double q2 = currentOrientation.y();
            double q3 = currentOrientation.z();

            // Normalize accelerometer measurement
            Vector3d a = accel.normalized();

            // Gradient descent algorithm corrective step
            double F_g[3];
            F_g[0] = 2 * (q1 * q3 - q0 * q2) - a.x();
            F_g[1] = 2 * (q0 * q1 + q2 * q3) - a.y();
            F_g[2] = 2 * (0.5 - q1 * q1 - q2 * q2) - a.z();

            // Compute gradient
            double J_g[3][4];
            J_g[0][0] = -2 * q2;
            J_g[0][1] = 2 * q3;
            J_g[0][2] = -2 * q0;
            J_g[0][3] = 2 * q1;
            J_g[1][0] = 2 * q1;
            J_g[1][1] = 2 * q0;
            J_g[1][2] = 2 * q3;
            J_g[1][3] = 2 * q2;
            J_g[2][0] = 0;
            J_g[2][1] = -4 * q1;
            J_g[2][2] = -4 * q2;
            J_g[2][3] = 0;

            // Compute step direction
            double step[4] = {0, 0, 0, 0};
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 4; j++) {
                    step[j] += J_g[i][j] * F_g[i];
                }
            }

            // Normalize step magnitude
            double stepMag = sqrt(step[0] * step[0] + step[1] * step[1] +
                                  step[2] * step[2] + step[3] * step[3]);
            if (stepMag > 1e-10) {
                for (int i = 0; i < 4; i++) {
                    step[i] /= stepMag;
                }
            }

            // Rate of change of quaternion from gyroscope
            double qDot[4];
            qDot[0] = 0.5 * (-q1 * gyro.x() - q2 * gyro.y() - q3 * gyro.z());
            qDot[1] = 0.5 * (q0 * gyro.x() + q2 * gyro.z() - q3 * gyro.y());
            qDot[2] = 0.5 * (q0 * gyro.y() - q1 * gyro.z() + q3 * gyro.x());
            qDot[3] = 0.5 * (q0 * gyro.z() + q1 * gyro.y() - q2 * gyro.x());

            // Compute and integrate final quaternion rate
            for (int i = 0; i < 4; i++) {
                qDot[i] -= beta * step[i];
            }

            // Integrate to get new orientation
            q0 += qDot[0] * dt;
            q1 += qDot[1] * dt;
            q2 += qDot[2] * dt;
            q3 += qDot[3] * dt;

            // Normalize quaternion
            double mag = sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
            currentOrientation = Quaterniond(q0 / mag, q1 / mag, q2 / mag, q3 / mag);
        }

        const Quaterniond &getOrientation() const override {
            return currentOrientation;
        }

        void reset() override {
            currentOrientation = Quaterniond::Identity();
        }

    private:
        double beta;
        Quaterniond currentOrientation;
    };
}



#endif //IMU_VISUALIZER_MADGWICK_FILTER_H
