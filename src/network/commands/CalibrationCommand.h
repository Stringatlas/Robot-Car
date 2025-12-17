#ifndef CALIBRATION_COMMAND_H
#define CALIBRATION_COMMAND_H

#include "ICommand.h"
#include "../../drive/DriveController.h"
#include "../../hardware/Encoder.h"
#include <functional>

/**
 * Blocking calibration command
 * Sweeps PWM values and records velocity data
 */
class CalibrationCommand : public ICommand {
public:
    struct Config {
        String motor;  // "left", "right", or "both"
        int startPWM;
        int endPWM;
        int stepSize;
        unsigned long holdTime;  // milliseconds to hold each PWM value
    };
    
    struct DataPoint {
        int pwm;
        float leftVelocity;
        float rightVelocity;
    };
    
    using DataCallback = std::function<void(const DataPoint&)>;
    using ProgressCallback = std::function<void(int current, int end, int start)>;
    using CompleteCallback = std::function<void()>;

private:
    DriveController* driveController;
    Encoder* leftEncoder;
    Encoder* rightEncoder;
    Config config;
    
    int currentPWM;
    unsigned long stepStartTime;
    bool active;
    
    DataCallback onDataPoint;
    ProgressCallback onProgress;
    CompleteCallback onComplete;

public:
    CalibrationCommand(DriveController* drive, Encoder* left, Encoder* right, const Config& cfg)
        : driveController(drive), leftEncoder(left), rightEncoder(right), 
          config(cfg), currentPWM(0), stepStartTime(0), active(false) {}
    
    bool start() override {
        currentPWM = config.startPWM;
        stepStartTime = millis();
        active = true;
        
        // Set initial PWM
        applyPWM(currentPWM);
        
        return true;
    }
    
    bool update() override {
        if (!active) return false;
        
        unsigned long now = millis();
        
        // Check if we've held this PWM long enough
        if (now - stepStartTime >= config.holdTime) {
            // Collect data point
            DataPoint point = {
                currentPWM,
                leftEncoder->getVelocity(),
                rightEncoder->getVelocity()
            };
            
            if (onDataPoint) onDataPoint(point);
            if (onProgress) onProgress(currentPWM, config.endPWM, config.startPWM);
            
            // Move to next step
            currentPWM += config.stepSize;
            
            if (currentPWM > config.endPWM) {
                // Calibration complete
                active = false;
                if (onComplete) onComplete();
                return false;  // Command finished
            }
            
            // Apply next PWM value
            applyPWM(currentPWM);
            stepStartTime = now;
        }
        
        return true;  // Still running
    }
    
    void stop() override {
        active = false;
        driveController->setLeftMotorPower(0);
        driveController->setRightMotorPower(0);
    }
    
    bool isBlocking() const override { return true; }
    const char* getName() const override { return "Calibration"; }
    bool isInterruptible() const override { return true; }  // Can be stopped by user
    
    // Set callbacks for progress updates
    void setDataCallback(DataCallback cb) { onDataPoint = cb; }
    void setProgressCallback(ProgressCallback cb) { onProgress = cb; }
    void setCompleteCallback(CompleteCallback cb) { onComplete = cb; }

private:
    void applyPWM(int pwm) {
        float pwmValue = pwm / 255.0f;
        
        if (config.motor == "left") {
            driveController->setLeftMotorPower(pwmValue);
            driveController->setRightMotorPower(0);
        } else if (config.motor == "right") {
            driveController->setLeftMotorPower(0);
            driveController->setRightMotorPower(pwmValue);
        } else if (config.motor == "both") {
            driveController->setLeftMotorPower(pwmValue);
            driveController->setRightMotorPower(pwmValue);
        }
    }
};

#endif
