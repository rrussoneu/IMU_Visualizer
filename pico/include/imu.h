#ifndef PICO_IMU_H
#define PICO_IMU_H

#include <array>
#include "hardware/i2c.h"
#include "hardware/gpio.h"  
#include "pico/stdlib.h"    

class IMU {
public:
    struct Data {
        std::array<float, 3> accel;
        std::array<float, 3> gyro;
    };

    IMU();
    bool init();
    bool read(Data& data);
    void calibrate();

private:
    bool write_register(uint8_t reg, uint8_t data);
    bool read_registers(uint8_t reg, uint8_t* buffer, size_t len);
    
    // Calibration offsets
    std::array<float, 3> accel_offset{0.0f, 0.0f, 0.0f};
    std::array<float, 3> gyro_offset{0.0f, 0.0f, 0.0f};
};

#endif // PICO_IMU_H