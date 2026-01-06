#ifndef COMMAND_FACTORY_H
#define COMMAND_FACTORY_H

#include "ICommand.h"
#include "JoystickCommand.h"
#include "DirectMotorCommand.h"
#include "VelocityCommand.h"
#include "CalibrationCommand.h"
#include "AutonomousSequenceCommand.h"
#include "../../drive/DriveController.h"
#include "../../drive/VelocityController.h"
#include "../../hardware/Encoder.h"
#include "../Telemetry.h"
#include <memory>

// C++11 compatibility - make_unique polyfill for older compilers
#if __cplusplus < 201402L
namespace std {
    template<typename T, typename... Args>
    unique_ptr<T> make_unique(Args&&... args) {
        return unique_ptr<T>(new T(forward<Args>(args)...));
    }
}
#endif

/**
 * Factory for creating command instances
 * Centralizes command creation logic
 */
class CommandFactory {
private:
    DriveController* driveController;
    VelocityController* velocityController;
    Encoder* leftEncoder;
    Encoder* rightEncoder;

public:
    CommandFactory(DriveController* drive, VelocityController* velCtrl, 
                   Encoder* left, Encoder* right)
        : driveController(drive), velocityController(velCtrl),
          leftEncoder(left), rightEncoder(right) {}
    
    std::unique_ptr<JoystickCommand> createJoystickCommand() {
        return std::make_unique<JoystickCommand>(driveController);
    }
    
    std::unique_ptr<DirectMotorCommand> createDirectMotorCommand(float left = 0, float right = 0) {
        return std::make_unique<DirectMotorCommand>(driveController, left, right);
    }
    
    std::unique_ptr<VelocityCommand> createVelocityCommand(float velocity) {
        return std::make_unique<VelocityCommand>(velocityController, velocity);
    }
    
    std::unique_ptr<CalibrationCommand> createCalibrationCommand(
        const CalibrationCommand::Config& config) {
        return std::make_unique<CalibrationCommand>(
            driveController, leftEncoder, rightEncoder, config);
    }
    
    std::unique_ptr<AutonomousSequenceCommand> createAutonomousSequence() {
        return std::make_unique<AutonomousSequenceCommand>(
            velocityController, leftEncoder, rightEncoder);
    }
    
    // Example: Create a pre-defined autonomous routine
    std::unique_ptr<AutonomousSequenceCommand> createSquarePattern(float sideLength) {
        auto cmd = createAutonomousSequence();
        
        for (int i = 0; i < 4; i++) {
            cmd->addDriveDistance(sideLength, 20.0f);  // Drive forward
            cmd->addWait(500);                         // Pause
            cmd->addTurnAngle(90.0f, 30.0f);          // Turn 90°
            cmd->addWait(500);
        }
        cmd->addStop();
        
        return cmd;
    }
    
    std::unique_ptr<AutonomousSequenceCommand> createFigureEight() {
        auto cmd = createAutonomousSequence();
        
        // First loop
        cmd->addDriveDistance(50, 20.0f);
        cmd->addTurnAngle(180, 30.0f);
        cmd->addDriveDistance(50, 20.0f);
        cmd->addTurnAngle(180, 30.0f);
        
        // Second loop (opposite direction)
        cmd->addTurnAngle(-180, 30.0f);
        cmd->addDriveDistance(50, 20.0f);
        cmd->addTurnAngle(-180, 30.0f);
        cmd->addDriveDistance(50, 20.0f);
        
        cmd->addStop();
        return cmd;
    }
};

#endif // COMMAND_FACTORY_H
