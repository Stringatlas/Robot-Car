# Command System Quick Reference

## Adding a New Command - 5 Steps

### 1. Create Command Class
```cpp
// src/network/commands/MyCommand.h
class MyCommand : public ICommand {
private:
    // Dependencies & state
public:
    MyCommand(/* inject dependencies */) { }
    
    bool start() override { /* init */ return true; }
    bool update() override { /* work */ return stillRunning; }
    void stop() override { /* cleanup */ }
    
    bool isBlocking() const override { return true/false; }
    const char* getName() const override { return "MyCommand"; }
};
```

### 2. Add to CommandFactory
```cpp
// CommandFactory.h
#include "MyCommand.h"

std::unique_ptr<MyCommand> createMyCommand(params) {
    return std::make_unique<MyCommand>(driveController, ...);
}
```

### 3. Handle in WebSocket
```cpp
// WebServer.cpp
else if (message == "MY_COMMAND") {
    commandExecutor.executeCommand(
        commandFactory->createMyCommand(params));
}
```

### 4. Update in Main Loop (already done!)
```cpp
// WebServer.cpp - update()
commandExecutor.update();  // Automatically runs your command
```

### 5. Test
```cpp
void testMyCommand() {
    MyCommand cmd(&drive, &encoder);
    cmd.start();
    while (cmd.update()) delay(10);
    cmd.stop();
}
```

---

## Common Patterns

### Pattern: Non-Blocking Real-Time Control
```cpp
class RealtimeCommand : public ICommand {
private:
    unsigned long lastUpdate;
    static constexpr unsigned long TIMEOUT_MS = 500;
    
public:
    bool update() override {
        if (millis() - lastUpdate > TIMEOUT_MS)
            return false;  // Timeout
        return true;  // Keep running
    }
    
    void updateData(float value) {
        // Apply value
        lastUpdate = millis();  // Reset timeout
    }
    
    bool isBlocking() const override { return false; }
};
```

### Pattern: Blocking Sequence
```cpp
class SequenceCommand : public ICommand {
private:
    int currentStep;
    std::vector<Step> steps;
    
public:
    bool update() override {
        if (steps[currentStep].isComplete()) {
            currentStep++;
            if (currentStep >= steps.size())
                return false;  // Done
            steps[currentStep].start();
        }
        steps[currentStep].update();
        return true;
    }
    
    bool isBlocking() const override { return true; }
};
```

### Pattern: Sensor-Based Loop
```cpp
class SensorCommand : public ICommand {
private:
    Sensor* sensor;
    float targetValue;
    
public:
    bool update() override {
        float current = sensor->read();
        
        if (abs(current - targetValue) < threshold)
            return false;  // Reached target
            
        adjustMotors(current, targetValue);
        return true;
    }
    
    bool isBlocking() const override { return true; }
};
```

### Pattern: Timed Action
```cpp
class TimedCommand : public ICommand {
private:
    unsigned long startTime;
    unsigned long duration;
    
public:
    bool start() override {
        startTime = millis();
        return true;
    }
    
    bool update() override {
        if (millis() - startTime >= duration)
            return false;  // Time's up
            
        // Do work
        return true;
    }
    
    bool isBlocking() const override { return true; }
};
```

### Pattern: Distance-Based Movement
```cpp
class DriveDistanceCommand : public ICommand {
private:
    Encoder* encoder;
    float startDistance;
    float targetDistance;
    
public:
    bool start() override {
        startDistance = encoder->getDistance();
        return true;
    }
    
    bool update() override {
        float traveled = encoder->getDistance() - startDistance;
        
        if (traveled >= targetDistance)
            return false;  // Done
            
        // Keep driving
        return true;
    }
    
    bool isBlocking() const override { return true; }
};
```

---

## Command System Cheat Sheet

### Execution
```cpp
// Start command
commandExecutor.executeCommand(std::move(cmd));

// Update in loop
commandExecutor.update();

// Stop current
commandExecutor.stopCurrentCommand();
```

### Checking State
```cpp
// Is anything running?
if (commandExecutor.isCommandRunning()) { }

// Is blocking command running?
if (commandExecutor.isBlockingCommandRunning()) { }

// What's running?
const char* name = commandExecutor.getCurrentCommandName();

// Get specific command type
auto* joystick = commandExecutor.getCurrentCommandAs<JoystickCommand>();
if (joystick) {
    joystick->updateJoystick(x, y);
}
```

### Creating Commands
```cpp
// Via factory
auto cmd = commandFactory->createJoystickCommand();
auto cmd = commandFactory->createVelocityCommand(30.0f);
auto cmd = commandFactory->createSquarePattern(100.0f);

// Direct construction
auto cmd = std::make_unique<CustomCommand>(dependencies);
```

### Callbacks
```cpp
// Progress updates
cmd->setProgressCallback([](int step, int total) {
    Serial.printf("Step %d/%d\n", step, total);
});

// Data streaming
cmd->setDataCallback([](const DataPoint& point) {
    sendToClient(point);
});

// Completion
cmd->setCompleteCallback([](bool success) {
    if (success) Serial.println("Done!");
});
```

---

## Decision Guide

### Should my command be blocking?

**YES (Blocking)** if:
- ✅ Runs a complete sequence to finish
- ✅ Robot is autonomous during execution
- ✅ User shouldn't control during command
- ✅ Examples: calibration, autonomous nav, routines

**NO (Non-blocking)** if:
- ✅ User maintains real-time control
- ✅ Command updates continuously from user input
- ✅ Runs until explicitly stopped or replaced
- ✅ Examples: joystick, velocity setpoint, manual control

### Should my command be interruptible?

**YES (Interruptible)** if:
- ✅ User should be able to stop it
- ✅ Safety requires ability to interrupt
- ✅ 99% of commands

**NO (Non-interruptible)** if:
- ✅ Must complete for safety (emergency stop)
- ✅ Must complete to prevent damage
- ✅ Very rare - use carefully!

---

## WebSocket Message Examples

```javascript
// Non-blocking commands
ws.send("JOYSTICK:0.5,0.8");           // Update joystick
ws.send("VELOCITY:30.0");               // Set velocity
ws.send("MOTORS:0.6,0.6");             // Direct motor control

// Blocking commands
ws.send("START_CALIBRATION:both,60,180,5,2000");
ws.send("AUTO_SQUARE");                 // Pre-built pattern
ws.send("AUTO_FIGURE8");                // Pre-built pattern
ws.send("AUTO_NAVIGATE:10.5,20.3");    // Custom autonomous

// Control
ws.send("STOP");                        // Stop current command
ws.send("REQUEST_CONTROL");             // Get control
ws.send("RELEASE_CONTROL");             // Release control
```

---

## File Structure

```
src/network/commands/
├── ICommand.h                    # Base interface
├── CommandExecutor.h             # Lifecycle manager  
├── CommandFactory.h              # Creation helper
│
├── JoystickCommand.h             # Non-blocking
├── VelocityCommand.h             # Non-blocking
├── DirectMotorCommand.h          # Non-blocking
│
├── CalibrationCommand.h          # Blocking
├── AutonomousSequenceCommand.h   # Blocking
│
└── YourCustomCommand.h           # Add yours here!
```

---

## Debugging Tips

### Enable logging
```cpp
#define COMMAND_DEBUG 1

#if COMMAND_DEBUG
  #define CMD_LOG(msg) TELEM_LOG(msg)
  #define CMD_LOGF(fmt, ...) TELEM_LOGF(fmt, __VA_ARGS__)
#else
  #define CMD_LOG(msg)
  #define CMD_LOGF(fmt, ...)
#endif
```

### Add to command
```cpp
bool update() override {
    CMD_LOGF("%s: step %d", getName(), currentStep);
    // ...
}
```

### Track executor state
```cpp
void CommandExecutor::executeCommand(...) {
    TELEM_LOGF("▶️ Starting: %s (%s)", 
        command->getName(),
        command->isBlocking() ? "blocking" : "non-blocking");
}
```

---

## Common Mistakes

### ❌ Forgetting to call update()
```cpp
// WRONG - executor never runs commands
void loop() {
    // Missing: commandExecutor.update();
}
```

### ❌ Not moving command to executor
```cpp
// WRONG - trying to use after move
auto cmd = factory->createCommand();
executor.executeCommand(std::move(cmd));
cmd->doSomething();  // ❌ cmd is now nullptr!
```

### ❌ Returning true forever
```cpp
// WRONG - command never completes
bool update() override {
    doWork();
    return true;  // ❌ Never returns false!
}
```

### ❌ Not checking if command started
```cpp
// BETTER - handle failure
if (!commandExecutor.executeCommand(std::move(cmd))) {
    TELEM_LOG("Failed to start command");
}
```

---

## Testing Template

```cpp
// Test individual command
void testCommand() {
    // Setup
    MockDrive drive;
    MockEncoder encoder;
    MyCommand cmd(&drive, &encoder);
    
    // Execute
    ASSERT_TRUE(cmd.start());
    
    int iterations = 0;
    while (cmd.update() && iterations++ < 1000) {
        delay(10);
    }
    
    // Verify
    ASSERT_TRUE(iterations < 1000);  // Didn't timeout
    ASSERT_EQ(expectedState, drive.getState());
    
    cmd.stop();
}
```

---

## Performance Considerations

### Command Creation
- ✅ Commands are lightweight (just pointers)
- ✅ Factory creates on-demand (no pre-allocation needed)
- ✅ Executor owns ONE command at a time

### Update Frequency
- ✅ Called once per main loop (~100Hz typical)
- ✅ Commands should complete quickly (<10ms)
- ✅ For slow operations, use state machines

### Memory
- ✅ No dynamic allocation during update()
- ✅ Command destroyed when complete (frees memory)
- ✅ Callbacks use std::function (small overhead)
