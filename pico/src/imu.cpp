#include "imu.h"
#include "config.h"
#include <cstring>
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

// MPU6050 registers
#define PWR_MGMT_1 0x6B
#define ACCEL_XOUT_H 0x3B
#define GYRO_XOUT_H 0x43
#define WHO_AM_I 0x75

IMU::IMU() {}

bool IMU::init() {
    // Initialize I2C
    i2c_init(I2C_PORT, I2C_FREQ);
    
    // Set up the GPIO pins
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    // Check device ID
    uint8_t who_am_i;
    if (!read_registers(WHO_AM_I, &who_am_i, 1) || who_am_i != 0x68) {
        return false;
    }
    
    // Wake up MPU6050
    if (!write_register(PWR_MGMT_1, 0x00)) {
        return false;
    }
    
    return true;
}

bool IMU::read(Data& data) {
    uint8_t buffer[14];
    
    if (!read_registers(ACCEL_XOUT_H, buffer, 14)) {
        return false;
    }
    
    // Convert raw data to float values
    for (int i = 0; i < 3; i++) {
        int16_t raw_accel = (buffer[i*2] << 8) | buffer[i*2 + 1];
        int16_t raw_gyro = (buffer[i*2 + 8] << 8) | buffer[i*2 + 9];
        
        // Convert to m/s*s
        data.accel[i] = (raw_accel / 16384.0f) * 9.81f - accel_offset[i];
        
        // Convert to rad/s
        data.gyro[i] = (raw_gyro / 131.0f) * (3.141592f / 180.0f) - gyro_offset[i];
    }
    
    return true;
}

void IMU::calibrate() {
    const int num_samples = 100;
    std::array<float, 3> accel_sum{0.0f, 0.0f, 0.0f};
    std::array<float, 3> gyro_sum{0.0f, 0.0f, 0.0f};
    
    // Collect samples
    for (int i = 0; i < num_samples; i++) {
        Data data;
        if (read(data)) {
            for (int j = 0; j < 3; j++) {
                accel_sum[j] += data.accel[j];
                gyro_sum[j] += data.gyro[j];
            }
        }
        sleep_ms(10);
    }
    
    // Calculate averages
    for (int i = 0; i < 3; i++) {
        accel_offset[i] = accel_sum[i] / num_samples;
        gyro_offset[i] = gyro_sum[i] / num_samples;
    }
    
    // Adjust accelerometer offset on z axis
    accel_offset[2] -= 9.81f;
}

bool IMU::write_register(uint8_t reg, uint8_t data) {
    uint8_t buffer[2] = {reg, data};
    return i2c_write_blocking(I2C_PORT, MPU6050_ADDR, buffer, 2, false) == 2;
}

bool IMU::read_registers(uint8_t reg, uint8_t* buffer, size_t len) {
    if (i2c_write_blocking(I2C_PORT, MPU6050_ADDR, &reg, 1, true) != 1) {
        return false;
    }
    return i2c_read_blocking(I2C_PORT, MPU6050_ADDR, buffer, len, false) == len;
}
