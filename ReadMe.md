# IMU Visualizer

This project is a real-time 3D visualization system that combines embedded systems with modern graphics programming. It uses a Qt interface with OpenGL to render a cube, controlling the orientation with an IMU. The IMU is connected to a Raspberry Pi Pico W for wireless sensor data transmission over TCP/IP. Sensor fusion algorithms (complementary, Madgwick, Kalman filters) were implemented to process the IMU data for smooth motion tracking. I'm currently working on implementing the main application on a Pi 4b as well. 

<img src="res/imu_cube.png">

<div style="display: flex; justify-content: center;">
  <a href="https://www.youtube.com/shorts/c-G2TD-YkFc" style="margin-right: 10px;">
    <img src="https://img.youtube.com/vi/c-G2TD-YkFc.jpg" alt="BLUEBERRY_JOKE" width="200">
  </a>
</div>