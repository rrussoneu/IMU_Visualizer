#ifndef PICO_IMU_CONFIG_H
#define PICO_IMU_CONFIG_H

// WiFi Configuration
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define SERVER_IP ""
#define SERVER_PORT 8080

// IMU Configuration
#define MPU6050_ADDR 0x68
#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5
#define I2C_FREQ 400000  // 400kHz

// Protocol Configuration
#define PACKET_START 0xAA
#define PACKET_END 0x55
#define PACKET_SIZE 26   // 1 start + 24 data + 1 end

// Sample rate configuration
#define SAMPLE_RATE_MS 10  // 100Hz update rate

#endif // PICO_IMU_CONFIG_H