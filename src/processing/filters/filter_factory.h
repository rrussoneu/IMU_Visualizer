//
// Created by Raphael Russo on 11/21/24.
//

#ifndef IMU_VISUALIZER_FILTER_FACTORY_H
#define IMU_VISUALIZER_FILTER_FACTORY_H
#include "complementary_filter.h"
#include "madgwick_filter.h"
#include "kalman_filter.h"
namespace imu_viz {
    class OrientationFilterFactory {
    public:
        enum class FilterType {
            COMPLEMENTARY,
            MADGWICK,
            KALMAN
        };

        static std::unique_ptr<IOrientationFilter> createFilter(FilterType type) {
            switch (type) {
                case FilterType::COMPLEMENTARY:
                    return std::make_unique<ComplementaryFilter>();
                case FilterType::MADGWICK:
                    return std::make_unique<MadgwickFilter>();
                case FilterType::KALMAN:
                    return std::make_unique<KalmanFilter>();
                default:
                    throw std::runtime_error("Unknown filter type");
            }
        }
    };
}
#endif //IMU_VISUALIZER_FILTER_FACTORY_H
