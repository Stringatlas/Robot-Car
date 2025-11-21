#ifndef AUTONOMOUS_SEQUENCE_COMMAND_H
#define AUTONOMOUS_SEQUENCE_COMMAND_H

#include "ICommand.h"
#include "../../drive/VelocityController.h"
#include "../../hardware/Encoder.h"
#include <vector>
#include <functional>

/**
 * Blocking autonomous sequence command
 * Executes a predefined sequence of movements
 * Example: Drive forward 1m, turn 90°, drive forward 0.5m, stop
 */
class AutonomousSequenceCommand : public ICommand {
public:
    enum class ActionType {
        DRIVE_DISTANCE,  // Drive forward/backward for X cm
        TURN_ANGLE,      // Turn in place X degrees
        DRIVE_TIME,      // Drive at velocity for X ms
        WAIT,            // Pause for X ms
        STOP             // Stop and end sequence
    };
    
    struct Action {
        ActionType type;
        float param1;  // Distance (cm), angle (deg), time (ms), or velocity (cm/s)
        float param2;  // Velocity for DRIVE_DISTANCE, or unused
    };
    
    using ProgressCallback = std::function<void(int stepIndex, int totalSteps)>;
    using CompleteCallback = std::function<void(bool success)>;

private:
    VelocityController* velocityController;
    Encoder* leftEncoder;
    Encoder* rightEncoder;
    
    std::vector<Action> sequence;
    size_t currentStep;
    unsigned long stepStartTime;
    float stepStartDistance;
    bool active;
    
    ProgressCallback onProgress;
    CompleteCallback onComplete;

public:
    AutonomousSequenceCommand(VelocityController* velCtrl, Encoder* left, Encoder* right)
        : velocityController(velCtrl), leftEncoder(left), rightEncoder(right),
          currentStep(0), stepStartTime(0), stepStartDistance(0), active(false) {}
    
    // Build the sequence
    void addDriveDistance(float distanceCm, float velocityCmPerS) {
        sequence.push_back({ActionType::DRIVE_DISTANCE, distanceCm, velocityCmPerS});
    }
    
    void addTurnAngle(float degrees, float angularVelocity = 30.0f) {
        sequence.push_back({ActionType::TURN_ANGLE, degrees, angularVelocity});
    }
    
    void addDriveTime(float velocityCmPerS, unsigned long timeMs) {
        sequence.push_back({ActionType::DRIVE_TIME, velocityCmPerS, (float)timeMs});
    }
    
    void addWait(unsigned long timeMs) {
        sequence.push_back({ActionType::WAIT, (float)timeMs, 0});
    }
    
    void addStop() {
        sequence.push_back({ActionType::STOP, 0, 0});
    }
    
    bool start() override {
        if (sequence.empty()) return false;
        
        currentStep = 0;
        active = true;
        startCurrentStep();
        return true;
    }
    
    bool update() override {
        if (!active || currentStep >= sequence.size()) {
            return false;
        }
        
        const Action& action = sequence[currentStep];
        bool stepComplete = false;
        
        switch (action.type) {
            case ActionType::DRIVE_DISTANCE: {
                velocityController->update();
                float currentDist = (leftEncoder->getDistance() + rightEncoder->getDistance()) / 2.0f;
                float traveled = abs(currentDist - stepStartDistance);
                stepComplete = (traveled >= abs(action.param1));
                break;
            }
            
            case ActionType::TURN_ANGLE: {
                // Simplified turn - in reality you'd use IMU or encoder differential
                velocityController->update();
                // For now, use time-based approximation
                // TODO: Implement proper angle tracking with IMU
                unsigned long elapsed = millis() - stepStartTime;
                float estimatedAngle = (elapsed / 1000.0f) * action.param2;
                stepComplete = (estimatedAngle >= abs(action.param1));
                break;
            }
            
            case ActionType::DRIVE_TIME: {
                velocityController->update();
                unsigned long elapsed = millis() - stepStartTime;
                stepComplete = (elapsed >= (unsigned long)action.param2);
                break;
            }
            
            case ActionType::WAIT: {
                unsigned long elapsed = millis() - stepStartTime;
                stepComplete = (elapsed >= (unsigned long)action.param1);
                break;
            }
            
            case ActionType::STOP:
                stepComplete = true;
                break;
        }
        
        if (stepComplete) {
            currentStep++;
            if (onProgress) onProgress(currentStep, sequence.size());
            
            if (currentStep >= sequence.size()) {
                active = false;
                if (onComplete) onComplete(true);
                return false;  // Sequence complete
            }
            
            startCurrentStep();
        }
        
        return true;
    }
    
    void stop() override {
        active = false;
        velocityController->setVelocity(0, 0);
        if (onComplete) onComplete(false);  // Stopped before completion
    }
    
    bool isBlocking() const override { return true; }
    const char* getName() const override { return "AutonomousSequence"; }
    bool isInterruptible() const override { return true; }
    
    void setProgressCallback(ProgressCallback cb) { onProgress = cb; }
    void setCompleteCallback(CompleteCallback cb) { onComplete = cb; }
    
    size_t getStepCount() const { return sequence.size(); }
    size_t getCurrentStep() const { return currentStep; }

private:
    void startCurrentStep() {
        if (currentStep >= sequence.size()) return;
        
        const Action& action = sequence[currentStep];
        stepStartTime = millis();
        
        switch (action.type) {
            case ActionType::DRIVE_DISTANCE:
                stepStartDistance = (leftEncoder->getDistance() + rightEncoder->getDistance()) / 2.0f;
                velocityController->setVelocity(action.param2, action.param2);
                break;
                
            case ActionType::TURN_ANGLE: {
                // Turn in place: one motor forward, one backward
                float turnVel = action.param2;
                if (action.param1 < 0) turnVel = -turnVel;  // Turn direction
                velocityController->setVelocity(turnVel, -turnVel);
                break;
            }
            
            case ActionType::DRIVE_TIME:
                velocityController->setVelocity(action.param1, action.param1);
                break;
                
            case ActionType::WAIT:
            case ActionType::STOP:
                velocityController->setVelocity(0, 0);
                break;
        }
    }
};

#endif // AUTONOMOUS_SEQUENCE_COMMAND_H
