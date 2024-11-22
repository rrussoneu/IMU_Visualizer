//
// Created by Raphael Russo on 11/21/24.
//

#ifndef IMU_VISUALIZER_DATA_PROCESSOR_H
#define IMU_VISUALIZER_DATA_PROCESSOR_H

#pragma once
#include "core/imu_data.h"
#include <QObject>
#include <deque>
#include "filters/orientation_filter.h"
#include "filters/filter_factory.h"

namespace imu_viz {

    class DataProcessor : public QObject {
    Q_OBJECT

    public:
        explicit DataProcessor(QObject *parent = nullptr);

        ~DataProcessor() override = default;
        DataProcessor(const DataProcessor&) = delete;
        DataProcessor& operator=(const DataProcessor&) = delete;
        DataProcessor(DataProcessor&&) noexcept = default;
        DataProcessor& operator=(DataProcessor&&) noexcept = default;

        void setFilterType(OrientationFilterFactory::FilterType type);
        void setCalibrationData(const CalibrationData &calibration);

    public slots:
        void processIMUData(const IMUData &data);
        void startCalibration();
        void finishCalibration();
        void resetOrientation();
        void updateCalibration(const IMUData& data);

    signals:
        void newOrientation(const Quaterniond &orientation);
        void newCalibrationData(const CalibrationData &calibration);
        void errorOccurred(const QString &error);

    private:
        static constexpr size_t CALIBRATION_SAMPLES = 1000;
        static constexpr double MIN_TIMESTAMP_DELTA = 0.001; // 1ms minimum between samples

        std::unique_ptr<IOrientationFilter> filter;
        CalibrationData calibration;
        bool isCalibrating{false};
        uint64_t lastTimestamp{0};

        // Mutex for thread safety
        mutable std::mutex dataMutex;

        // Calibration buffers
        std::deque<Vector3d> accelBuffer;
        std::deque<Vector3d> gyroBuffer;

        Vector3d applyCalibration(const Vector3d &raw,
                                  const Vector3d &bias,
                                  const Matrix3d &scale) const;

        bool validateIMUData(const IMUData& data) const;
        void updateOrientation(const Vector3d& accel, const Vector3d& gyro, double deltaTime);
    };
}

#endif //IMU_VISUALIZER_DATA_PROCESSOR_H
