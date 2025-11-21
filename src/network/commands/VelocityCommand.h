#ifndef VELOCITY_COMMAND_H
#define VELOCITY_COMMAND_H

#include "ICommand.h"
#include "../../drive/VelocityController.h"

/**
 * Non-blocking velocity control command
 * Sets target velocity for closed-loop control
 */
class VelocityCommand : public ICommand {
private:
    VelocityController* velocityController;
    float targetVelocity;
    unsigned long lastUpdateTime;
    static constexpr unsigned long TIMEOUT_MS = 500;

public:
    VelocityCommand(VelocityController* velCtrl, float velocity)
        : velocityController(velCtrl), targetVelocity(velocity), lastUpdateTime(0) {}
    
    bool start() override {
        velocityController->setVelocity(targetVelocity, targetVelocity);
        lastUpdateTime = millis();
        return true;
    }
    
    bool update() override {
        // Let velocity controller handle the control loop
        velocityController->update();
        
        // Timeout check
        if (millis() - lastUpdateTime > TIMEOUT_MS) {
            return false;
        }
        
        return true;
    }
    
    void stop() override {
        velocityController->setVelocity(0, 0);
    }
    
    bool isBlocking() const override { return false; }
    const char* getName() const override { return "Velocity"; }
    bool isInterruptible() const override { return true; }
    
    void updateVelocity(float velocity) {
        targetVelocity = velocity;
        velocityController->setVelocity(velocity, velocity);
        lastUpdateTime = millis();
    }
};

#endif // VELOCITY_COMMAND_H
