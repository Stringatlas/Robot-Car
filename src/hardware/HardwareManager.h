#ifndef HARDWAREMANAGER_H
#define HARDWAREMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Encoder.h"
#include "BatteryMonitor.h"
#include "IMU.h"

class WebSocketHandler;

class HardwareManager {
public:
    static HardwareManager& getInstance();
    
    void begin();
    void update();
    void broadcastTelemetry(WebSocketHandler* wsHandler);
    
    Encoder* getLeftEncoder() { return &leftEncoder; }
    Encoder* getRightEncoder() { return &rightEncoder; }
    BatteryMonitor* getBatteryMonitor() { return &batteryMonitor; }
    IMU* getIMU() { return &imu; }
    
    void resetEncoders();
    bool isIMUCalibrated() const;

private:
    HardwareManager();
    HardwareManager(const HardwareManager&) = delete;
    HardwareManager& operator=(const HardwareManager&) = delete;
    
    Encoder leftEncoder;
    Encoder rightEncoder;
    BatteryMonitor batteryMonitor;
    IMU imu;
    
    unsigned long lastBroadcastTime;
    static constexpr unsigned long BROADCAST_INTERVAL_MS = 200;
    
    bool hasSignificantChange();
    
    float lastVoltage;
    long lastLeftCount;
    long lastRightCount;
    float lastHeading;
    
    static const int IMU_CALIBRATION_SAMPLES = 1000;
    static constexpr float VOLTAGE_THRESHOLD = 0.1;
    static constexpr long COUNT_THRESHOLD = 5;
    static constexpr float HEADING_THRESHOLD = 0.05;
};

#endif
