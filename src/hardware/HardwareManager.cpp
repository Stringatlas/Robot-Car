#include "HardwareManager.h"
#include "../network/WebSocketHandler.h"
#include "../network/Telemetry.h"
#include "config.h"

HardwareManager& HardwareManager::getInstance() {
    static HardwareManager instance;
    return instance;
}

HardwareManager::HardwareManager()
    : leftEncoder(LEFT_ENCODER_A, LEFT_ENCODER_B, ENCODER_PPR, WHEEL_DIAMETER, false),
      rightEncoder(RIGHT_ENCODER_A, RIGHT_ENCODER_B, ENCODER_PPR, WHEEL_DIAMETER, true),
      batteryMonitor(BATTERY_VOLTAGE_PIN, BATTERY_VOLTAGE_MULTIPLIER),
      imu(IMU_CALIBRATION_SAMPLES),
      lastBroadcastTime(0),
      lastVoltage(0),
      lastLeftCount(0),
      lastRightCount(0),
      lastHeading(0) {}

void HardwareManager::begin() {
    Encoder::registerLeft(&leftEncoder);
    Encoder::registerRight(&rightEncoder);
    
    leftEncoder.begin();
    rightEncoder.begin();
    TELEM_LOG_INFO("Encoders initialized");
    
    batteryMonitor.begin();
    TELEM_LOG_INFO("Battery monitor initialized");
    
    if (imu.begin()) {
        TELEM_LOG_INFO("IMU initialized - calibrating...");
        imu.calibrate();
        TELEM_LOG_INFO("IMU calibration complete");
    } else {
        TELEM_LOG_ERROR("IMU initialization failed");
    }
}

void HardwareManager::update() {
    leftEncoder.update();
    rightEncoder.update();
    
    if (imu.isCalibrated()) {
        imu.update();
    }
}

void HardwareManager::broadcastTelemetry(WebSocketHandler* wsHandler) {
    if (!wsHandler) return;
    
    unsigned long now = millis();
    if (now - lastBroadcastTime < BROADCAST_INTERVAL_MS) {
        return;
    }
    
    if (wsHandler->getClientCount() == 0) {
        return;
    }
    
    if (!hasSignificantChange()) {
        return;
    }
    
    StaticJsonDocument<512> doc;
    doc["type"] = "telemetry";
    
    JsonObject encoders = doc.createNestedObject("encoders");
    JsonObject left = encoders.createNestedObject("left");
    left["count"] = leftEncoder.getCount();
    left["revolutions"] = leftEncoder.getRevolutions();
    left["distance"] = leftEncoder.getDistance();
    left["velocity"] = leftEncoder.getVelocity();
    left["rpm"] = leftEncoder.getRPM();
    
    JsonObject right = encoders.createNestedObject("right");
    right["count"] = rightEncoder.getCount();
    right["revolutions"] = rightEncoder.getRevolutions();
    right["distance"] = rightEncoder.getDistance();
    right["velocity"] = rightEncoder.getVelocity();
    right["rpm"] = rightEncoder.getRPM();
    
    doc["battery"]["voltage"] = batteryMonitor.getVoltage();
    
    if (imu.isCalibrated()) {
        JsonObject imuData = doc.createNestedObject("imu");
        imuData["heading"] = imu.getHeading();
        imuData["headingDegrees"] = imu.getHeadingDegrees();
        imuData["gyroZ"] = imu.getGyroZ();
        imuData["accelX"] = imu.getAccelX();
        imuData["accelY"] = imu.getAccelY();
        imuData["accelZ"] = imu.getAccelZ();
    }
    
    wsHandler->broadcastJson(doc);
    
    lastVoltage = batteryMonitor.getVoltage();
    lastLeftCount = leftEncoder.getCount();
    lastRightCount = rightEncoder.getCount();
    lastHeading = imu.getHeading();
    lastBroadcastTime = now;
}

bool HardwareManager::hasSignificantChange() {
    float currentVoltage = batteryMonitor.getVoltage();
    long currentLeftCount = leftEncoder.getCount();
    long currentRightCount = rightEncoder.getCount();
    float currentHeading = imu.isCalibrated() ? imu.getHeading() : 0;
    
    if (abs(currentVoltage - lastVoltage) > VOLTAGE_THRESHOLD) return true;
    if (abs(currentLeftCount - lastLeftCount) > COUNT_THRESHOLD) return true;
    if (abs(currentRightCount - lastRightCount) > COUNT_THRESHOLD) return true;
    if (imu.isCalibrated() && abs(currentHeading - lastHeading) > HEADING_THRESHOLD) return true;
    
    return false;
}

void HardwareManager::resetEncoders() {
    leftEncoder.reset();
    rightEncoder.reset();
    lastLeftCount = 0;
    lastRightCount = 0;
    TELEM_LOG_INFO("Encoders reset");
}

bool HardwareManager::isIMUCalibrated() const {
    return imu.isCalibrated();
}
