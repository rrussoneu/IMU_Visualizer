//
// Created by Raphael Russo on 11/21/24.
//

#pragma once
#include "data_processor.h"

#include <stdexcept>

namespace imu_viz {
    DataProcessor::DataProcessor(QObject* parent)
            : QObject(parent)
            , filter(OrientationFilterFactory::createFilter(
                    OrientationFilterFactory::FilterType::KALMAN))
    {
        if (!filter) {
            throw std::runtime_error("Failed to create orientation filter");
        }

    }

    void DataProcessor::setFilterType(OrientationFilterFactory::FilterType type) {
        std::lock_guard<std::mutex> lock(dataMutex);
        auto newFilter = OrientationFilterFactory::createFilter(type);
        if (!newFilter) {
            emit errorOccurred("Failed to create new filter");
            return;
        }
        filter = std::move(newFilter);
    }


    void DataProcessor::processIMUData(const IMUData& data) {
        if (!validateIMUData(data)) {
            emit errorOccurred("Invalid IMU data received");
            return;
        }

        std::lock_guard<std::mutex> lock(dataMutex);

        // Calculate time delta
        double deltaTime = 0.0;
        if (lastTimestamp != 0) {
            deltaTime = static_cast<double>(data.timestamp - lastTimestamp) / 1000000.0; // Convert to seconds
            if (deltaTime < MIN_TIMESTAMP_DELTA) {
                return; // Skip updates that are too close together
            }
        }
        lastTimestamp = data.timestamp;

        try {
            // Scale down the raw values
            const double ACCEL_SCALE = 0.1;  // Reduce acceleration sensitivity
            const double GYRO_SCALE = 0.1;   // Reduce gyroscope sensitivity

            // Scale and apply calibration
            Vector3d scaledAccel = data.acceleration * ACCEL_SCALE;
            Vector3d scaledGyro = data.gyroscope * GYRO_SCALE;

            if (filter) {
                // Update orientation using scaled values
                filter->update(scaledAccel, scaledGyro, deltaTime);

                // Get the filtered orientation
                Quaterniond currentOrientation = filter->getOrientation();

                // Apply smoothing
                static Quaterniond lastOrientation = currentOrientation;
                const double SMOOTH_FACTOR = 0.7;

                // Slerp between last and current orientation
                Quaterniond smoothedOrientation = lastOrientation.slerp(SMOOTH_FACTOR, currentOrientation);
                lastOrientation = smoothedOrientation;

                emit DataProcessor::newOrientation(smoothedOrientation);
            }
        } catch (const std::exception& e) {
            emit DataProcessor::errorOccurred(QString("Orientation update error: %1").arg(e.what()));
        }
    }

    void DataProcessor::startCalibration() {
        std::lock_guard<std::mutex> lock(dataMutex);
        isCalibrating = true;
        accelBuffer.clear();
        gyroBuffer.clear();
    }

    void DataProcessor::updateCalibration(const IMUData& data) {
        std::lock_guard<std::mutex> lock(dataMutex);
        if (!isCalibrating) return;

        // Add new samples to buffers
        accelBuffer.push_back(data.acceleration);
        gyroBuffer.push_back(data.gyroscope);

        // Keep buffer size limited
        while (accelBuffer.size() > CALIBRATION_SAMPLES) {
            accelBuffer.pop_front();
        }
        while (gyroBuffer.size() > CALIBRATION_SAMPLES) {
            gyroBuffer.pop_front();
        }
    }

    void DataProcessor::finishCalibration() {
        std::lock_guard<std::mutex> lock(dataMutex);

        if (accelBuffer.size() < CALIBRATION_SAMPLES ||
            gyroBuffer.size() < CALIBRATION_SAMPLES) {
            emit errorOccurred("Not enough samples for calibration");
            return;
        }

        try {
            // Calculate accelerometer stats
            Vector3d accelMean = Vector3d::Zero();
            Matrix3d accelCovariance = Matrix3d::Zero();

            // Mean
            for (const auto& accel : accelBuffer) {
                accelMean += accel;
            }
            accelMean /= static_cast<double>(accelBuffer.size());

            // Covariance
            for (const auto& accel : accelBuffer) {
                Vector3d diff = accel - accelMean;
                accelCovariance += diff * diff.transpose();
            }
            accelCovariance /= static_cast<double>(accelBuffer.size() - 1);

            // Gyroscope stats
            Vector3d gyroMean = Vector3d::Zero();
            Matrix3d gyroCovariance = Matrix3d::Zero();

            // Mean
            for (const auto& gyro : gyroBuffer) {
                gyroMean += gyro;
            }
            gyroMean /= static_cast<double>(gyroBuffer.size());

            // Covariance
            for (const auto& gyro : gyroBuffer) {
                Vector3d diff = gyro - gyroMean;
                gyroCovariance += diff * diff.transpose();
            }
            gyroCovariance /= static_cast<double>(gyroBuffer.size() - 1);

            // Update calibration
            CalibrationData newCalibration;

            // For accelerometer assuming device is stationary and experiencing 1g
            newCalibration.accelBias = accelMean - Vector3d(0, 0, 9.81);

            // For gyroscope also assume stationary
            newCalibration.gyroBias = gyroMean;

            Vector3d accelScaleDiag = Vector3d::Ones() + 0.5 * accelCovariance.diagonal();
            newCalibration.accelScale = accelScaleDiag.asDiagonal();

            // For gyroscope scaling
            Vector3d gyroScaleDiag = Vector3d::Ones() + 0.5 * gyroCovariance.diagonal();
            newCalibration.gyroScale = gyroScaleDiag.asDiagonal();

            calibration = newCalibration;
            emit newCalibrationData(calibration);

        } catch (const std::exception& e) {
            emit errorOccurred(QString("Calibration calculation error: %1").arg(e.what()));
        }

        // Reset state
        isCalibrating = false;
        accelBuffer.clear();
        gyroBuffer.clear();
    }

    void DataProcessor::resetOrientation() {
        std::lock_guard<std::mutex> lock(dataMutex);
        if (filter) {
            filter->reset();
        }
    }

    void DataProcessor::setCalibrationData(const CalibrationData& newCalibration) {
        std::lock_guard<std::mutex> lock(dataMutex);
        calibration = newCalibration;
    }

    Vector3d DataProcessor::applyCalibration(const Vector3d& raw,
                                             const Vector3d& bias,
                                             const Matrix3d& scale) const
    {
        return scale * (raw - bias);
    }

    bool DataProcessor::validateIMUData(const IMUData& data) const {
        for (int i = 0; i < 3; ++i) {
            if (!std::isfinite(data.acceleration[i]) || !std::isfinite(data.gyroscope[i])) {
                return false;
            }
        }

        // Check for acceleration
        const double accelMagnitude = data.acceleration.norm();
        if (accelMagnitude < 0.981 || accelMagnitude > 39.24) {
            return false;
        }

        // Gyroscrope/ angular magnitude check
        const double gyroMagnitude = data.gyroscope.norm();
        if (gyroMagnitude > 8.726) { // 500 deg/s in rad/s
            return false;
        }

        return true;
    }

    void DataProcessor::updateOrientation(const Vector3d& accel, const Vector3d& gyro, double deltaTime) {
        if (!filter) return;

        try {
            filter->update(accel, gyro, deltaTime);
            emit newOrientation(filter->getOrientation());
        } catch (const std::exception& e) {
            emit errorOccurred(QString("Orientation update error: %1").arg(e.what()));
        }
    }
}