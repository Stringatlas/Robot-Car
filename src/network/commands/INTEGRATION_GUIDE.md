# Command Architecture Integration Guide

## Overview
This document shows how to integrate the new command-based architecture into WebServer.cpp to support scalable, extensible robot control.

## Architecture Benefits

### 1. **Separation of Concerns**
- Commands encapsulate behavior (what to do)
- Executor manages lifecycle (when/how to run)
- WebServer handles communication (user interface)

### 2. **Easy to Extend**
- Add new autonomous routines without modifying WebServer
- Create complex sequences by composing commands
- Commands are reusable across different interfaces (WebSocket, API, autonomous scheduler)

### 3. **Type Safety**
- Blocking vs non-blocking is explicit
- Interruptibility is explicit
- No more manual mode tracking

## Integration Example

### In WebServer.h - Add these members:

```cpp
#include "commands/CommandExecutor.h"
#include "commands/CommandFactory.h"

class WebServerManager {
private:
    // ... existing members ...
    
    // NEW: Command system
    CommandExecutor commandExecutor;
    std::unique_ptr<CommandFactory> commandFactory;
    
    // Remove old mode tracking (replaced by commands):
    // ❌ ControlMode currentMode;
    // ❌ bool calibrationActive;
    // ❌ calibration state variables...
};
```

### In WebServer.cpp - WebSocket Handler

Replace mode-based handling with command execution:

```cpp
// OLD WAY (mode-based):
if (message.startsWith("JOYSTICK:")) {
    setControlMode(ControlMode::JOYSTICK);
    driveController->setPowerControl(y, x);
}

// NEW WAY (command-based):
if (message.startsWith("JOYSTICK:")) {
    float x, y;
    // ... parse x, y ...
    
    // Get or create joystick command
    auto* joystickCmd = commandExecutor.getCurrentCommandAs<JoystickCommand>();
    if (!joystickCmd) {
        // No joystick command running, start one
        commandExecutor.executeCommand(commandFactory->createJoystickCommand());
        joystickCmd = commandExecutor.getCurrentCommandAs<JoystickCommand>();
    }
    
    // Update joystick position
    if (joystickCmd) {
        joystickCmd->updateJoystick(x, y);
    }
}

// Calibration example:
else if (message.startsWith("START_CALIBRATION:")) {
    // Parse calibration parameters
    CalibrationCommand::Config config = parseCalibrationParams(message);
    
    // Create and execute calibration command
    auto calibCmd = commandFactory->createCalibrationCommand(config);
    
    // Set up callbacks for progress updates
    calibCmd->setDataCallback([this](const CalibrationCommand::DataPoint& point) {
        ws.textAll(WebSocketMessageBuilder::buildCalibrationPoint(
            point.pwm, point.leftVelocity, point.rightVelocity));
    });
    
    calibCmd->setProgressCallback([this](int curr, int end, int start) {
        ws.textAll(WebSocketMessageBuilder::buildCalibrationProgress(curr, end, start));
    });
    
    calibCmd->setCompleteCallback([this]() {
        ws.textAll("CALIBRATION_COMPLETE");
        TELEM_LOG("✓ Calibration complete");
    });
    
    if (!commandExecutor.executeCommand(std::move(calibCmd))) {
        TELEM_LOG("❌ Failed to start calibration");
    }
}

// Autonomous sequence example:
else if (message == "AUTO_SQUARE") {
    auto squareCmd = commandFactory->createSquarePattern(100.0f);  // 100cm sides
    
    squareCmd->setProgressCallback([this](int step, int total) {
        ws.textAll(WebSocketMessageBuilder::buildProgress(step, total));
    });
    
    squareCmd->setCompleteCallback([this](bool success) {
        ws.textAll(success ? "AUTO_COMPLETE" : "AUTO_STOPPED");
    });
    
    commandExecutor.executeCommand(std::move(squareCmd));
}

else if (message == "AUTO_FIGURE8") {
    commandExecutor.executeCommand(commandFactory->createFigureEight());
}

// Velocity control:
else if (message.startsWith("VELOCITY:")) {
    float velocity = parseVelocity(message);
    
    auto* velCmd = commandExecutor.getCurrentCommandAs<VelocityCommand>();
    if (!velCmd) {
        // Start new velocity command
        commandExecutor.executeCommand(
            commandFactory->createVelocityCommand(velocity));
    } else {
        // Update existing velocity command
        velCmd->updateVelocity(velocity);
    }
}

// Stop any command:
else if (message == "STOP") {
    commandExecutor.stopCurrentCommand();
}
```

### In WebServer.cpp - Main Update Loop

Replace mode state machine with command executor:

```cpp
void WebServerManager::update() {
    // OLD WAY:
    // switch (currentMode) {
    //     case ControlMode::CALIBRATION: updateCalibration(); break;
    //     case ControlMode::VELOCITY: velocityController->update(); break;
    //     ...
    // }
    
    // NEW WAY:
    commandExecutor.update();  // That's it! Commands handle themselves.
}
```

## Adding New Commands

### Example: Wall Following Autonomous Mode

```cpp
// 1. Create WallFollowCommand.h
class WallFollowCommand : public ICommand {
private:
    VelocityController* velCtrl;
    // ... ultrasonic sensor, PID controller, etc ...
    
public:
    bool start() override {
        // Initialize sensors, start following
    }
    
    bool update() override {
        // Read distance sensor
        // Adjust velocity to maintain wall distance
        // Return true until stopped
    }
    
    void stop() override {
        // Stop motors
    }
    
    bool isBlocking() const override { return true; }
    const char* getName() const override { return "WallFollow"; }
};

// 2. Add to CommandFactory
std::unique_ptr<WallFollowCommand> createWallFollowCommand() {
    return std::make_unique<WallFollowCommand>(velocityController);
}

// 3. Use in WebSocket handler
else if (message == "AUTO_WALL_FOLLOW") {
    commandExecutor.executeCommand(commandFactory->createWallFollowCommand());
}
```

## Command Types Comparison

| Command Type | Blocking | Typical Use | Can be Interrupted |
|-------------|----------|-------------|-------------------|
| JoystickCommand | No | Real-time control | Yes |
| VelocityCommand | No | Set-and-forget velocity | Yes |
| DirectMotorCommand | No | Manual motor testing | Yes |
| CalibrationCommand | Yes | Data collection | Yes |
| AutonomousSequenceCommand | Yes | Scripted movements | Yes |
| WallFollowCommand | Yes | Continuous autonomous | Yes |
| EmergencyStopCommand | Yes | Safety | No (non-interruptible) |

## Migration Strategy

### Phase 1: Add commands alongside existing code
- Keep existing mode-based system working
- Add CommandExecutor as new member
- Create commands for new features only

### Phase 2: Migrate one mode at a time
- Start with easiest: Calibration → CalibrationCommand
- Then: Joystick → JoystickCommand
- Then: Velocity → VelocityCommand

### Phase 3: Remove old mode system
- Delete ControlMode enum
- Remove setControlMode(), checkControlTimeout()
- Clean up state variables

## Testing Commands

Commands are much easier to test:

```cpp
// Test calibration without WebServer
void testCalibration() {
    CalibrationCommand::Config config{
        .motor = "left",
        .startPWM = 60,
        .endPWM = 180,
        .stepSize = 10,
        .holdTime = 1000
    };
    
    CalibrationCommand cmd(&drive, &leftEnc, &rightEnc, config);
    
    cmd.start();
    while (cmd.update()) {
        delay(10);  // Simulate main loop
    }
    cmd.stop();
}
```

## Future Extensions

### Command Scheduling
```cpp
// Run commands at specific times
commandScheduler.scheduleAt(8, 0, 0, 
    commandFactory->createSquarePattern(50.0f));
```

### Command Chaining
```cpp
// Run commands in sequence
commandChain
    .add(commandFactory->createVelocityCommand(30))
    .waitFor(5000)
    .add(commandFactory->createTurnAngle(90))
    .add(commandFactory->createVelocityCommand(30))
    .execute();
```

### Conditional Commands
```cpp
// Run commands based on sensor data
if (ultrasonicDistance < 20.0f) {
    commandExecutor.executeCommand(
        commandFactory->createAvoidObstacle());
}
```
