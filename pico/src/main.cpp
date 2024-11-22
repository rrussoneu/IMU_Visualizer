#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "config.h"
#include "imu.h"
#include "network.h"

// LED stuff
void set_led_status(bool connected) {
    if (connected) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    } else {
        // Blink LED when disconnected
        static uint32_t last_blink = 0;
        uint32_t now = time_us_32();
        if (now - last_blink > 500000) {  // 500ms
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 
                !cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN));
            last_blink = now;
        }
    }
}

int main() {
    stdio_init_all();
    
    printf("Pico W IMU Starting...\n");
    
    IMU imu;
    Network network;
    
    // Initialize IMU
    if (!imu.init()) {
        printf("Failed to initialize IMU\n");
        return -1;
    }
    
    printf("IMU initialized\n");
    
    // Calibrate IMU
    printf("Calibrating IMU (keep device still)...\n");
    imu.calibrate();
    printf("Calibration complete\n");
    
    // Initialize network
    if (!network.init()) {
        printf("Failed to initialize network\n");
        return -1;
    }
    
    printf("Connecting to WiFi...\n");
    if (!network.connect_wifi()) {
        printf("Failed to connect to WiFi\n");
        return -1;
    }
    
    printf("Connected to WiFi, connecting to server...\n");
    if (!network.connect_tcp()) {
        printf("Failed to connect to server\n");
        return -1;
    }
    
    printf("Connected to server, starting main loop\n");
    
    // Main loop
    while (true) {
        // Update LED status
        set_led_status(network.get_status() == Network::Status::CONNECTED);
        
        // Read IMU data
        IMU::Data imu_data;
        if (imu.read(imu_data)) {
            // Create packet
            uint8_t packet[PACKET_SIZE];
            packet[0] = PACKET_START;
            
            // Copy accelerometer data
            memcpy(&packet[1], imu_data.accel.data(), 12);
            
            // Copy gyroscope data
            memcpy(&packet[13], imu_data.gyro.data(), 12);
            
            packet[PACKET_SIZE-1] = PACKET_END;
            
            // Send the data
            if (!network.send_data(packet, PACKET_SIZE)) {
                printf("Failed to send data, reconnecting...\n");
                network.connect_tcp();
            }
        }
        
        sleep_ms(SAMPLE_RATE_MS);
    }
    
    return 0;
}