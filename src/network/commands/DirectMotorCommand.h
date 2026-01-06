#ifndef DIRECT_MOTOR_COMMAND_H
#define DIRECT_MOTOR_COMMAND_H

#include "ICommand.h"
#include "../../drive/DriveController.h"

/**
 * Non-blocking direct motor power command
 * Sets raw motor power values without any control loop
 */
class DirectMotorCommand : public ICommand {
private:
    DriveController* driveController;
    float leftPower, rightPower;
    unsigned long lastUpdateTime;
    static constexpr unsigned long TIMEOUT_MS = 500;

public:
    DirectMotorCommand(DriveController* drive, float left = 0, float right = 0)
        : driveController(drive), leftPower(left), rightPower(right), lastUpdateTime(0) {}
    
    bool start() override {
        driveController->setLeftMotorPower(leftPower);
        driveController->setRightMotorPower(rightPower);
        lastUpdateTime = millis();
        return true;
    }
    
    bool update() override {
        // Non-blocking command - runs until replaced or timeout
        if (millis() - lastUpdateTime > TIMEOUT_MS) {
            return false;  // Timeout
        }
        return true;
    }
    
    void stop() override {
        driveController->setLeftMotorPower(0);
        driveController->setRightMotorPower(0);
    }
    
    bool isBlocking() const override { return false; }
    const char* getName() const override { return "DirectMotor"; }
    bool isInterruptible() const override { return true; }
    
    // Update motor powers (called from WebSocket handler)
    void setMotorPowers(float left, float right) {
        leftPower = left;
        rightPower = right;
        driveController->setLeftMotorPower(left);
        driveController->setRightMotorPower(right);
        lastUpdateTime = millis();
    }
};

#endif
