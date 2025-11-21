#ifndef IMU_H
#define IMU_H

#include <Wire.h>
#include <MPU6050.h>

class IMU {
public:
    IMU(int calibrationSamples = 1000);
    
    bool begin();
    void calibrate();
    void update();
    
    float getHeading() const { return heading; } // in radians
    float getHeadingDegrees() const { return heading * 180.0 / PI; }
    float getGyroZ() const { return gyroZ; }    // in rad/s
    float getAccelX() const { return accelX; }  // in m/s^2
    float getAccelY() const { return accelY; }  // in m/s^2
    float getAccelZ() const { return accelZ; }  // in m/s^2
    
    bool isCalibrated() const { return calibrated; }

private:
    MPU6050 mpu;
    int calibrationSamples;
    bool calibrated;
    
    float gyroZBias;
    float accelXBias;
    float accelYBias;
    float accelZBias;
    
    float gyroZ;
    float accelX;
    float accelY;
    float accelZ;
    
    float accelXFiltered;
    float accelYFiltered;
    float accelZFiltered;
    
    float heading;
    unsigned long lastUpdateTime;
    
    static constexpr float ACCEL_SCALE = 16384.0 / 9.81;
    static constexpr float GYRO_SCALE = 131.0;
    static constexpr float ACCEL_ALPHA = 0.1;
};

#endif
