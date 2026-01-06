# ESP32 Robot Car

ESP32-S3 robot car with encoders, L298N motor driver, IMU, battery monitor, and web interface.

## Features

- Wheel encoders with velocity control
- L298N motor driver
- IMU (MPU6050)
- Battery voltage monitoring
- WebSocket communication
- Web dashboard
- Command pattern architecture
- Telemetry logging

## Hardware

- ESP32-S3-DevKitM-1
- Wheel encoders
- L298N motor driver
- MPU6050 IMU
- Battery monitor

## Build

```bash
pio run
pio run --target upload
pio run --target uploadfs
```

## Usage

Configure WiFi in `include/config.h`, upload firmware and filesystem, access web dashboard at ESP32 IP address.
