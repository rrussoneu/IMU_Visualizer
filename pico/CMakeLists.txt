# Generated Cmake Pico project file
cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialize environment settings
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()

# Set board type to Pico W
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Include Pico SDK
include(pico_sdk_import.cmake)
project(pico_imu_wireless C CXX ASM)
pico_sdk_init()

# Create list of source files
set(SOURCE_FILES
        ../../../Desktop/src/main.cpp
        ../../../Desktop/src/imu.cpp
        ../../../Desktop/src/network.cpp
)

# Create list of header files
set(HEADER_FILES
        ../../../Desktop/include/config.h
        ../../../Desktop/include/imu.h
        ../../../Desktop/include/network.h
)

# Add executable
add_executable(pico_imu_wireless
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

# Set program name and version
pico_set_program_name(pico_imu_wireless "pico_imu_wireless")
pico_set_program_version(pico_imu_wireless "0.1")

# Configure stdio output
pico_enable_stdio_uart(pico_imu_wireless 0)
pico_enable_stdio_usb(pico_imu_wireless 1)

# Set include directories
target_include_directories(pico_imu_wireless PRIVATE
        ../../../Desktop
        ../../../Desktop/include
)

# Link libraries
target_link_libraries(pico_imu_wireless
    pico_stdlib
    hardware_i2c
    hardware_dma
    hardware_timer
    hardware_watchdog
    hardware_clocks
    pico_cyw43_arch_lwip_threadsafe_background
)

# Create additional outputs (uf2, bin, hex)
pico_add_extra_outputs(pico_imu_wireless)