# Command Architecture Summary

## What We've Built

A **Command Pattern** architecture that replaces your mode-based system with a more scalable, extensible design.

## Core Components

### 1. **ICommand Interface** - Base for all commands
```cpp
class ICommand {
    virtual bool start() = 0;      // Initialize
    virtual bool update() = 0;     // Run iteration (return false when done)
    virtual void stop() = 0;       // Cleanup
    virtual bool isBlocking() = 0; // Prevents other commands?
    virtual const char* getName() = 0;
};
```

### 2. **CommandExecutor** - Manages command lifecycle
- Starts/stops commands
- Handles blocking vs non-blocking
- Manages interruption rules
- Called in main loop

### 3. **CommandFactory** - Creates commands with dependencies
- Centralizes command creation
- Injects hardware dependencies
- Provides pre-built autonomous routines

### 4. **Concrete Commands** - Do the actual work

#### Non-Blocking (instant control):
- ✅ `JoystickCommand` - Real-time manual driving
- ✅ `VelocityCommand` - Velocity setpoint control
- ✅ `DirectMotorCommand` - Raw motor power

#### Blocking (run to completion):
- ✅ `CalibrationCommand` - PWM sweep & data collection
- ✅ `AutonomousSequenceCommand` - Scripted movements (square, figure-8, custom)

## How It Works

```
User Action (WebSocket)
        ↓
WebServer parses message
        ↓
CommandFactory creates command
        ↓
CommandExecutor.executeCommand(cmd)
        ↓
        ├─→ Interrupts current command (if allowed)
        └─→ Starts new command
        
Main Loop:
    commandExecutor.update()
        └─→ Calls command.update()
            └─→ Returns false when done
                └─→ Auto-stops and cleans up
```

## Comparison: Old vs New

### Old Mode-Based System
```cpp
enum ControlMode { IDLE, JOYSTICK, VELOCITY, CALIBRATION };
ControlMode currentMode;

void onWebSocket(message) {
    if (message == "JOYSTICK:...") {
        currentMode = JOYSTICK;
        // Parse and apply
    } else if (message == "START_CALIBRATION:...") {
        currentMode = CALIBRATION;
        calibrationActive = true;
        // Set up state variables
    }
    // ... many more conditions
}

void update() {
    switch(currentMode) {
        case CALIBRATION: updateCalibration(); break;
        case VELOCITY: velocityController->update(); break;
        // ... handle each mode
    }
    checkTimeout();
}
```

**Problems:**
- ❌ All logic in WebServer (600 lines)
- ❌ Hard to add new autonomous modes
- ❌ Can't test commands independently
- ❌ State spread across many variables
- ❌ Timeout logic duplicated

### New Command-Based System
```cpp
void onWebSocket(message) {
    if (message == "JOYSTICK:...") {
        auto cmd = factory->createJoystickCommand();
        executor.executeCommand(std::move(cmd));
    } else if (message == "START_CALIBRATION:...") {
        auto cmd = factory->createCalibrationCommand(config);
        cmd->setDataCallback([](data) { /* send to client */ });
        executor.executeCommand(std::move(cmd));
    } else if (message == "AUTO_SQUARE") {
        executor.executeCommand(factory->createSquarePattern(100));
    } else if (message == "AUTO_FIGURE8") {
        executor.executeCommand(factory->createFigureEight());
    }
}

void update() {
    executor.update();  // That's it!
}
```

**Benefits:**
- ✅ Commands are self-contained (~50-100 lines each)
- ✅ Easy to add new autonomous modes (just new command class)
- ✅ Testable in isolation
- ✅ State encapsulated in command
- ✅ Timeout handled by command

## Key Design Decisions

### 1. Blocking vs Non-Blocking
- **Non-blocking**: User maintains direct control (joystick, velocity)
- **Blocking**: Robot executes a plan (calibration, autonomous)
- **Enforced at runtime**: Executor prevents conflicts

### 2. Interruptibility
- All commands start as interruptible by default
- Can mark as non-interruptible for safety-critical operations
- Example: Emergency stop is non-interruptible

### 3. Ownership
- `CommandExecutor` owns current command
- Uses `std::unique_ptr` for memory safety
- Commands auto-destroyed when replaced/completed

### 4. Callbacks
- Commands notify progress through callbacks
- Decouples command logic from UI/telemetry
- Example: Calibration sends data points as they're collected

## What You Can Build Now

### Simple Autonomous Routines
```cpp
auto cmd = factory->createAutonomousSequence();
cmd->addDriveDistance(100, 20);   // Forward 100cm at 20cm/s
cmd->addTurnAngle(90, 30);        // Turn 90° at 30°/s
cmd->addDriveDistance(50, 20);    // Forward 50cm
cmd->addStop();
executor.executeCommand(std::move(cmd));
```

### Pre-Built Patterns
```cpp
executor.executeCommand(factory->createSquarePattern(100));
executor.executeCommand(factory->createFigureEight());
```

### Future: Complex Behaviors
```cpp
// Obstacle avoidance
class ObstacleAvoidCommand : public ICommand {
    bool update() override {
        if (ultrasonicSensor.getDistance() < 20) {
            // Back up and turn
        } else {
            // Drive forward
        }
    }
};

// Line following
class LineFollowCommand : public ICommand {
    bool update() override {
        // Read line sensors
        // Adjust steering
    }
};

// Wall following
class WallFollowCommand : public ICommand {
    bool update() override {
        // Maintain constant distance from wall
    }
};
```

## Next Steps

### Option A: Gradual Migration
1. Keep existing WebServer.cpp working
2. Add CommandExecutor as new member
3. Migrate one feature at a time (start with calibration)
4. Remove old code once everything migrated

### Option B: Fresh Start
1. Backup current WebServer.cpp
2. Rewrite using command system from scratch
3. Much cleaner but more work upfront

### Option C: Hybrid
1. Use commands for **new** features only
2. Keep existing features in old system
3. Slowly migrate as time permits

## Files Created

```
src/network/commands/
├── README.md                      # This overview
├── INTEGRATION_GUIDE.md           # Step-by-step integration
├── ICommand.h                     # Base interface
├── CommandExecutor.h              # Lifecycle manager
├── CommandFactory.h               # Command creation
├── JoystickCommand.h              # Manual control
├── VelocityCommand.h              # Velocity control
├── DirectMotorCommand.h           # Raw motor control
├── CalibrationCommand.h           # Calibration routine
└── AutonomousSequenceCommand.h    # Autonomous movements
```

## Why This Architecture?

### Scalability ⚡
- Add new autonomous modes without modifying WebServer
- Commands are independent modules
- Easy to compose complex behaviors

### Testability 🧪
- Test commands without WebServer
- Mock hardware dependencies
- Unit test each command in isolation

### Maintainability 🛠️
- Each command ~50-100 lines (vs 600-line WebServer)
- Clear separation of concerns
- Changes isolated to relevant command

### Extensibility 🔮
- Command scheduling
- Command chaining
- Conditional execution
- Recording/playback
- Multi-robot coordination

## Questions?

- **"Do I have to rewrite everything?"** No, migrate gradually
- **"Will this work with my current WebSocket protocol?"** Yes, minimal changes needed
- **"Can I mix old and new systems?"** Yes during migration
- **"How do I add a new autonomous mode?"** Create new command class, add to factory
- **"What about safety?"** Commands can be non-interruptible, executor enforces rules

## Recommendation

**Start with CalibrationCommand migration** because:
1. Most self-contained (~50 lines of calibration logic)
2. Blocking behavior clearly defined
3. Easy to test success
4. Good learning example for other commands
5. Immediate benefit: CalibrationCommand is reusable elsewhere

See `INTEGRATION_GUIDE.md` for code examples!
