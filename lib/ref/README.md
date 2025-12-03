# Robot Command Architecture

A scalable, extensible command pattern implementation for robot control that supports both blocking (autonomous routines, calibration) and non-blocking (joystick, velocity) commands.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                        WebServer                             │
│  ┌────────────────────────────────────────────────────────┐ │
│  │         WebSocket/HTTP Command Handler                 │ │
│  └────────────────┬───────────────────────────────────────┘ │
│                   │ sends commands                           │
│                   ▼                                          │
│  ┌────────────────────────────────────────────────────────┐ │
│  │            CommandExecutor                             │ │
│  │  • Manages command lifecycle                           │ │
│  │  • Handles blocking/non-blocking                       │ │
│  │  • Controls interruption                               │ │
│  └────────────────┬───────────────────────────────────────┘ │
│                   │ executes                                 │
│                   ▼                                          │
│  ┌────────────────────────────────────────────────────────┐ │
│  │            ICommand (interface)                        │ │
│  └────────────────┬───────────────────────────────────────┘ │
│                   │ implemented by                           │
└───────────────────┼──────────────────────────────────────────┘
                    │
        ┌───────────┴────────────────────────────┐
        │                                        │
        ▼                                        ▼
┌──────────────────┐                  ┌──────────────────────┐
│ Non-Blocking     │                  │ Blocking             │
│ Commands         │                  │ Commands             │
├──────────────────┤                  ├──────────────────────┤
│ • JoystickCommand│                  │ • CalibrationCommand │
│ • VelocityCommand│                  │ • AutonomousSequence │
│ • DirectMotor    │                  │ • WallFollowCommand  │
│                  │                  │ • ObstacleAvoid      │
└──────────────────┘                  └──────────────────────┘
```

## Command Types

### Non-Blocking Commands
**Characteristics:**
- Return control immediately
- Run until replaced by another command or timeout
- Can be updated in real-time (e.g., joystick position)
- Multiple can exist conceptually, but only one active at a time

**Examples:**
- `JoystickCommand` - Real-time manual control
- `VelocityCommand` - Set velocity and let PID handle it
- `DirectMotorCommand` - Raw motor power for testing

### Blocking Commands
**Characteristics:**
- Run a complete sequence to completion
- Prevent other commands from starting (unless interrupted)
- Report progress through callbacks
- Ideal for autonomous routines

**Examples:**
- `CalibrationCommand` - Sweep PWM values and collect data
- `AutonomousSequenceCommand` - Execute a series of movements
- `WallFollowCommand` - Continuous sensor-based navigation

## Key Interfaces

### ICommand
All commands implement this interface:

```cpp
class ICommand {
public:
    virtual bool start() = 0;           // Initialize command
    virtual bool update() = 0;          // Run one iteration (returns false when done)
    virtual void stop() = 0;            // Clean up command
    virtual bool isBlocking() const = 0;       // Blocks other commands?
    virtual bool isInterruptible() const = 0;  // Can be stopped early?
    virtual const char* getName() const = 0;   // For logging
};
```

### CommandExecutor
Manages command lifecycle:

```cpp
commandExecutor.executeCommand(std::move(command));  // Start command
commandExecutor.update();                            // Call in main loop
commandExecutor.stopCurrentCommand();                // Emergency stop
```

### CommandFactory
Creates commands with proper dependencies:

```cpp
commandFactory->createJoystickCommand();
commandFactory->createCalibrationCommand(config);
commandFactory->createSquarePattern(sideLength);
commandFactory->createFigureEight();
```

## Usage Examples

### Example 1: Manual Joystick Control

```cpp
// User moves joystick
if (message.startsWith("JOYSTICK:")) {
    float x, y;  // Parse from message
    
    // Get existing joystick command or create new one
    auto* cmd = commandExecutor.getCurrentCommandAs<JoystickCommand>();
    if (!cmd) {
        commandExecutor.executeCommand(commandFactory->createJoystickCommand());
        cmd = commandExecutor.getCurrentCommandAs<JoystickCommand>();
    }
    cmd->updateJoystick(x, y);
}
```

### Example 2: Run Calibration

```cpp
// User starts calibration
CalibrationCommand::Config config{
    .motor = "both",
    .startPWM = 60,
    .endPWM = 180,
    .stepSize = 5,
    .holdTime = 2000
};

auto calibCmd = commandFactory->createCalibrationCommand(config);

// Set up progress callbacks
calibCmd->setDataCallback([](const auto& point) {
    Serial.printf("PWM: %d, L: %.2f, R: %.2f\n", 
                  point.pwm, point.leftVelocity, point.rightVelocity);
});

calibCmd->setCompleteCallback([]() {
    Serial.println("Calibration complete!");
});

commandExecutor.executeCommand(std::move(calibCmd));
```

### Example 3: Autonomous Square Pattern

```cpp
// Create 1m x 1m square
auto squareCmd = commandFactory->createSquarePattern(100.0f);

squareCmd->setProgressCallback([](int step, int total) {
    Serial.printf("Step %d of %d\n", step, total);
});

commandExecutor.executeCommand(std::move(squareCmd));
```

### Example 4: Custom Autonomous Sequence

```cpp
auto sequence = commandFactory->createAutonomousSequence();

// Drive forward 50cm
sequence->addDriveDistance(50.0f, 20.0f);

// Wait 1 second
sequence->addWait(1000);

// Turn 90 degrees
sequence->addTurnAngle(90.0f, 30.0f);

// Drive forward another 30cm
sequence->addDriveDistance(30.0f, 15.0f);

// Stop
sequence->addStop();

commandExecutor.executeCommand(std::move(sequence));
```

### Example 5: Interrupt Current Command

```cpp
// User hits emergency stop
if (message == "EMERGENCY_STOP") {
    commandExecutor.stopCurrentCommand();
    // All motors automatically stopped
}
```

## Creating New Commands

### Step 1: Create Command Class

```cpp
// MyNewCommand.h
class MyNewCommand : public ICommand {
private:
    // Your state variables
    
public:
    MyNewCommand(/* dependencies */) { }
    
    bool start() override {
        // Initialize your command
        return true;
    }
    
    bool update() override {
        // Do work each iteration
        // Return false when complete
        return stillRunning;
    }
    
    void stop() override {
        // Clean up
    }
    
    bool isBlocking() const override { return true; }  // or false
    const char* getName() const override { return "MyCommand"; }
};
```

### Step 2: Add to CommandFactory

```cpp
std::unique_ptr<MyNewCommand> createMyCommand() {
    return std::make_unique<MyNewCommand>(/* deps */);
}
```

### Step 3: Use in WebSocket Handler

```cpp
else if (message == "MY_COMMAND") {
    commandExecutor.executeCommand(commandFactory->createMyCommand());
}
```

## Benefits

### 1. **Scalability**
- Add new autonomous routines without touching WebServer
- Commands are self-contained and reusable
- Easy to compose complex behaviors

### 2. **Testability**
- Test commands independently of WebServer
- Mock dependencies easily
- Unit test command logic in isolation

### 3. **Maintainability**
- Clear separation of concerns
- Each command has single responsibility
- No giant switch statements

### 4. **Safety**
- Explicit blocking behavior prevents conflicts
- Timeout handling built into non-blocking commands
- Emergency stop works consistently

### 5. **Extensibility**
- Add command scheduling
- Implement command chaining
- Create conditional commands
- Build complex autonomous behaviors

## Command State Machine

```
┌──────────┐
│   IDLE   │ ◄─────────────────────┐
└────┬─────┘                       │
     │ executeCommand()            │
     ▼                             │
┌──────────┐                       │
│ STARTING │                       │
└────┬─────┘                       │
     │ start() returns true        │
     ▼                             │
┌──────────┐                       │
│ RUNNING  │                       │
│          │                       │
│ update() │◄──┐                   │
│  called  │   │                   │
│  in loop │   │ returns true      │
└────┬─────┘   │                   │
     │         │                   │
     │ returns false OR            │
     │ stopCurrentCommand()        │
     ▼                             │
┌──────────┐                       │
│ STOPPING │                       │
└────┬─────┘                       │
     │ stop() called               │
     │                             │
     └─────────────────────────────┘
```

## Files

- `ICommand.h` - Base interface for all commands
- `CommandExecutor.h` - Command lifecycle manager
- `CommandFactory.h` - Command creation with dependencies
- `JoystickCommand.h` - Real-time joystick control
- `VelocityCommand.h` - Velocity setpoint control
- `DirectMotorCommand.h` - Raw motor power control
- `CalibrationCommand.h` - Motor calibration routine
- `AutonomousSequenceCommand.h` - Scripted movement sequences
- `INTEGRATION_GUIDE.md` - How to integrate with WebServer

## Future Ideas

### Command Scheduling
```cpp
scheduler.scheduleAt(8, 0, 0, commandFactory->createSquarePattern());
scheduler.scheduleEvery(1000, commandFactory->createBatteryCheck());
```

### Command Chaining
```cpp
chain.add(cmd1).then(cmd2).then(cmd3).onComplete(callback);
```

### Sensor-Triggered Commands
```cpp
if (ultrasonicSensor.getDistance() < 20.0f) {
    commandExecutor.executeCommand(commandFactory->createAvoidObstacle());
}
```

### Command Recording/Playback
```cpp
recorder.start();
// User drives robot manually
recorder.stop();
recorder.playback();  // Replays the movement
```

## Migration Path

1. ✅ Create command infrastructure (this step)
2. Add CommandExecutor to WebServer
3. Migrate one command type at a time:
   - Calibration → CalibrationCommand
   - Joystick → JoystickCommand  
   - Velocity → VelocityCommand
4. Remove old mode-based system
5. Add new autonomous commands

See `INTEGRATION_GUIDE.md` for detailed migration steps.
