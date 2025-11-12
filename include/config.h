#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration (Station Mode)
#define WIFI_SSID "ATTXIw8xjT_Guest"
#define WIFI_PASSWORD "guestWifi123@"

// OTA Configuration
#define OTA_HOSTNAME "ESP32-RobotCar"
#define OTA_PASSWORD "robotcar"

// Encoder Configuration
#define LEFT_ENCODER_A 43
#define LEFT_ENCODER_B 44
#define RIGHT_ENCODER_A 4
#define RIGHT_ENCODER_B 5

#define ENCODER_PPR 960  // 240 PPR × 4 (quadrature) = 960 counts per revolution
#define WHEEL_DIAMETER 5 // in cm

// Motor Driver Configuration (L298N)
// Left Motor
#define MOTOR_IN3 2
#define MOTOR_IN4 1
#define MOTOR_ENB 42   

// Right Motor
#define MOTOR_IN1 7
#define MOTOR_IN2 6
#define MOTOR_ENA 15   

#define MOTOR_PWM_FREQ 1000  // 1 kHz PWM frequency
#define MOTOR_PWM_RESOLUTION 8  // 8-bit resolution (0-255)
#define MOTOR_LEFT_PWM_CHANNEL 0
#define MOTOR_RIGHT_PWM_CHANNEL 1

// Battery Voltage Reader
#define BATTERY_VOLTAGE_PIN 9    // ADC1 pin (GPIO 1-10 work with WiFi on ESP32-S3)
#define BATTERY_VOLTAGE_MULTIPLIER 6.1  // 6.1/1 voltage divider ratio

#define WEB_SERVER_PORT 80


#endif
