#include "IMU.h"
#include "config.h"

IMU::IMU(int calibrationSamples) 
    : calibrationSamples(calibrationSamples),
      calibrated(false),
      gyroZBias(0),
      accelXBias(0),
      accelYBias(0),
      accelZBias(0),
      gyroZ(0),
      accelX(0),
      accelY(0),
      accelZ(0),
      accelXFiltered(0),
      accelYFiltered(0),
      accelZFiltered(0),
      heading(0),
      lastUpdateTime(0) {
}

bool IMU::begin() {
    Wire.begin(IMU_SDA, IMU_SCL);
    
    mpu.initialize();
    
    if (!mpu.testConnection()) {
        return false;
    }
    
    mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_250);
    mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
    
    lastUpdateTime = millis();
    return true;
}

void IMU::calibrate() {
    int16_t ax, ay, az, gx, gy, gz;
    long axSum = 0, aySum = 0, azSum = 0, gzSum = 0;
    
    for (int i = 0; i < calibrationSamples; i++) {
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
        
        axSum += ax;
        aySum += ay;
        azSum += az;
        gzSum += gz;
        
        delay(2);
    }
    
    accelXBias = (float)axSum / calibrationSamples;
    accelYBias = (float)aySum / calibrationSamples;
    accelZBias = (float)azSum / calibrationSamples;
    gyroZBias = (float)gzSum / calibrationSamples;
    
    calibrated = true;
    heading = 0;
    lastUpdateTime = millis();
}

void IMU::update() {
    if (!calibrated) return;
    
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    
    accelX = (ax - accelXBias) / ACCEL_SCALE;
    accelY = (ay - accelYBias) / ACCEL_SCALE;
    accelZ = (az - accelZBias) / ACCEL_SCALE;
    
    accelXFiltered = ACCEL_ALPHA * accelX + (1.0 - ACCEL_ALPHA) * accelXFiltered;
    accelYFiltered = ACCEL_ALPHA * accelY + (1.0 - ACCEL_ALPHA) * accelYFiltered;
    accelZFiltered = ACCEL_ALPHA * accelZ + (1.0 - ACCEL_ALPHA) * accelZFiltered;
    
    gyroZ = (gz - gyroZBias) / GYRO_SCALE;
    
    unsigned long currentTime = millis();
    float dt = (currentTime - lastUpdateTime) / 1000.0;
    lastUpdateTime = currentTime;
    
    heading += (gyroZ * PI / 180.0) * dt;
    
    // Normalize heading to [-PI, PI]
    while (heading > PI) heading -= 2 * PI;
    while (heading < -PI) heading += 2 * PI;
}
