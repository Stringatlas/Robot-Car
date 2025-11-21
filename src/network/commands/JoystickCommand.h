#ifndef JOYSTICK_COMMAND_H
#define JOYSTICK_COMMAND_H

#include "ICommand.h"
#include "../../drive/DriveController.h"

/**
 * Non-blocking joystick control command
 * Updates motor powers based on joystick input (x=turn, y=forward)
 */
class JoystickCommand : public ICommand {
private:
    DriveController* driveController;
    float x, y;  // Current joystick position
    unsigned long lastUpdateTime;
    static constexpr unsigned long TIMEOUT_MS = 500;

public:
    JoystickCommand(DriveController* drive) 
        : driveController(drive), x(0), y(0), lastUpdateTime(0) {}
    
    bool start() override {
        lastUpdateTime = millis();
        return true;
    }
    
    bool update() override {
        // Non-blocking commands don't auto-complete, they run until replaced
        // But we do timeout to IDLE if no updates received
        if (millis() - lastUpdateTime > TIMEOUT_MS) {
            return false;  // Timeout, command should end
        }
        
        // Apply joystick control (already set via updateJoystick)
        return true;
    }
    
    void stop() override {
        // Stop motors when command ends
        driveController->setLeftMotorPower(0);
        driveController->setRightMotorPower(0);
    }
    
    bool isBlocking() const override { return false; }
    const char* getName() const override { return "Joystick"; }
    bool isInterruptible() const override { return true; }
    
    // Update joystick position (called from WebSocket handler)
    void updateJoystick(float newX, float newY) {
        x = newX;
        y = newY;
        lastUpdateTime = millis();
        driveController->setPowerControl(y, x);
    }
    
    bool isAtCenter() const {
        return (abs(x) < 0.01f && abs(y) < 0.01f);
    }
};

#endif // JOYSTICK_COMMAND_H
