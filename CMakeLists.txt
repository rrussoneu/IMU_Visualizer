cmake_minimum_required(VERSION 3.16)
project(imu_visualizer VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required packages
find_package(Qt6 COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets REQUIRED)
find_package(Eigen3 REQUIRED)

# Platform-specific settings
if(APPLE)
    add_definitions(-DGL_DESKTOP)
elseif(RASPBERRY_PI)
    add_definitions(-DGL_ES)
endif()

# Source files
set(SOURCES
        src/main.cpp
)

# Resource files
set(RESOURCES
   #resources/resources.qrc
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${RESOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
        Eigen3::Eigen
)

# Installation
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
)