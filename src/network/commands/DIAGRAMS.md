# Command Architecture Diagrams

## System Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                             USER INTERFACE                               │
│                    (Web Browser, Mobile App, etc.)                      │
└──────────────────────────────┬──────────────────────────────────────────┘
                               │ WebSocket Messages
                               │ "JOYSTICK:0.5,0.8"
                               │ "AUTO_SQUARE"
                               │ "START_CALIBRATION:..."
                               ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                          WebServerManager                                │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │              WebSocket Handler                                   │   │
│  │  • Parses incoming messages                                      │   │
│  │  • Creates commands via factory                                  │   │
│  │  • Sends telemetry/progress to clients                          │   │
│  └───────────────────────────┬─────────────────────────────────────┘   │
│                              │                                           │
│  ┌───────────────────────────▼─────────────────────────────────────┐   │
│  │              CommandFactory                                       │   │
│  │  createJoystickCommand()                                         │   │
│  │  createCalibrationCommand(config)                                │   │
│  │  createSquarePattern(size)                                       │   │
│  │  createAutonomousSequence()                                      │   │
│  └───────────────────────────┬─────────────────────────────────────┘   │
│                              │ Creates & injects dependencies            │
│  ┌───────────────────────────▼─────────────────────────────────────┐   │
│  │              CommandExecutor                                      │   │
│  │  • executeCommand(cmd) ─ Start new command                       │   │
│  │  • update() ──────────── Called every loop iteration            │   │
│  │  • stopCurrentCommand() ─ Emergency stop                         │   │
│  │  • Enforces blocking/interruptible rules                        │   │
│  └───────────────────────────┬─────────────────────────────────────┘   │
└────────────────────────────────┼──────────────────────────────────────────┘
                                │ Delegates to current command
                                ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         ICommand Interface                               │
│  start() ─ Initialize command                                           │
│  update() ─ Run one iteration (returns false when complete)            │
│  stop() ─ Cleanup                                                       │
│  isBlocking() ─ Prevents other commands?                               │
│  getName() ─ For logging                                                │
└──────────────┬──────────────────────────────────┬───────────────────────┘
               │                                  │
     ┌─────────┴─────────┐           ┌───────────┴────────────┐
     │ Non-Blocking      │           │ Blocking               │
     │ Commands          │           │ Commands               │
     └─────────┬─────────┘           └───────────┬────────────┘
               │                                  │
     ┌─────────┼─────────────────┐      ┌────────┼──────────────────┐
     │         │                 │      │        │                  │
     ▼         ▼                 ▼      ▼        ▼                  ▼
┌─────────┐ ┌─────────┐  ┌──────────┐ ┌──────────┐  ┌──────────────────┐
│Joystick │ │Velocity │  │DirectMoto│ │Calibrat. │  │AutonomousSequence│
│Command  │ │Command  │  │rCommand  │ │Command   │  │Command           │
└────┬────┘ └────┬────┘  └─────┬────┘ └────┬─────┘  └────┬─────────────┘
     │           │              │           │             │
     └───────────┴──────────────┴───────────┴─────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                      Hardware Controllers                                │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐                 │
│  │DriveControl. │  │VelocityCtrl  │  │Encoders      │                 │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘                 │
│         │                  │                  │                          │
│         └──────────────────┴──────────────────┘                         │
│                            │                                             │
│                            ▼                                             │
│                    ┌──────────────┐                                     │
│                    │ Motor Drivers│                                     │
│                    └──────────────┘                                     │
└─────────────────────────────────────────────────────────────────────────┘
```

## Command Lifecycle State Machine

```
                    ┌──────────────────────────────┐
                    │    No Command Running        │
                    │       (IDLE STATE)           │
                    └──────────────┬───────────────┘
                                   │
                                   │ executeCommand(cmd)
                                   │
                                   ▼
                    ┌──────────────────────────────┐
                    │  Check Current Command       │
                    │  • Interruptible?            │
                    │  • Blocking?                 │
                    └──────────────┬───────────────┘
                                   │
                     ┌─────────────┴─────────────┐
                     │                           │
        Can Interrupt│                           │Cannot Interrupt
                     ▼                           ▼
        ┌────────────────────┐      ┌──────────────────────┐
        │ Stop Old Command   │      │ Reject New Command   │
        │ currentCmd->stop() │      │ Return false         │
        └──────────┬─────────┘      └──────────────────────┘
                   │
                   ▼
        ┌────────────────────┐
        │ Start New Command  │
        │ cmd->start()       │
        └──────────┬─────────┘
                   │
                   │ Returns true
                   ▼
        ┌────────────────────────────────┐
        │     RUNNING STATE              │
        │                                │
        │  Every Loop Iteration:         │
        │    cmd->update()               │◄────┐
        │                                │     │
        └──────────────┬─────────────────┘     │
                       │                       │
          ┌────────────┴────────────┐          │
          │                         │          │
   Returns│true                     │Returns   │
   (keep  │                         │false     │
   running)                         │(done)    │
          │                         │          │
          └─────────────────────────┘          │
                                    │          │
                                    ▼          │
                       ┌────────────────────┐  │
                       │ Stop & Cleanup     │  │
                       │ cmd->stop()        │  │
                       │ Release command    │  │
                       └──────────┬─────────┘  │
                                  │            │
                                  ▼            │
                       ┌────────────────────┐  │
                       │  Back to IDLE      │  │
                       └────────────────────┘  │
                                               │
                       User calls              │
                       stopCurrentCommand() ───┘
```

## Command Type Decision Tree

```
                        Need robot control?
                               │
                ┌──────────────┴──────────────┐
                │                             │
         Real-time control              Pre-planned sequence
         (User in the loop)            (Robot autonomous)
                │                             │
                ▼                             ▼
        Non-Blocking Command          Blocking Command
                │                             │
    ┌───────────┼───────────┐    ┌───────────┼──────────┐
    │           │           │    │           │          │
    ▼           ▼           ▼    ▼           ▼          ▼
Joystick    Velocity    Direct  Calibr.   Autonomous  Custom
 Control     Target     Motor   Sweep     Sequence    Routine
                        Test
    │           │           │    │           │          │
    ▼           ▼           ▼    ▼           ▼          ▼
Joystick    Velocity    Direct  Calibr.   Autonomous  YourCommand
Command     Command     Motor   Command   Sequence    Class
                       Command            Command
```

## Message Flow Example: Starting Calibration

```
1. User clicks "Start Calibration" in web UI
                │
                ▼
2. JavaScript sends WebSocket message:
   "START_CALIBRATION:both,60,180,5,2000"
                │
                ▼
3. WebServer.onWebSocketEvent() receives message
                │
                ▼
4. Parse parameters into CalibrationCommand::Config
                │
                ▼
5. CommandFactory.createCalibrationCommand(config)
                │
                ▼
6. Set up callbacks for progress/data/complete
                │
                ▼
7. CommandExecutor.executeCommand(calibCmd)
                │
                ├─→ Stops any running command
                ├─→ Calls calibCmd->start()
                └─→ Stores as currentCommand
                │
                ▼
8. Main loop calls commandExecutor.update()
                │
                ▼
9. Executor calls calibCmd->update()
                │
                ├─→ Checks if PWM hold time elapsed
                ├─→ Reads encoder velocities
                ├─→ Fires dataCallback() ───→ Sends data to web UI
                ├─→ Increments PWM
                └─→ Returns true (still running)
                │
                ▼
10. Repeat step 8-9 until all PWM values swept
                │
                ▼
11. calibCmd->update() returns false
                │
                ▼
12. Executor calls calibCmd->stop()
                │
                ▼
13. Fires completeCallback() ───→ Shows "Complete!" in web UI
                │
                ▼
14. Executor releases command (back to IDLE)
```

## Comparison: Adding New Autonomous Mode

### OLD WAY (Mode-Based):
```
1. Add new enum value to ControlMode
                │
                ▼
2. Add state variables to WebServerManager
   (startTime, targetDistance, currentStep, etc.)
                │
                ▼
3. Add case to WebSocket handler
   (100+ lines of parsing/validation)
                │
                ▼
4. Add case to update() switch statement
   (50+ lines of state machine logic)
                │
                ▼
5. Add case to setControlMode()
   (cleanup logic)
                │
                ▼
6. Test entire WebServer
   (All 600 lines at risk)
```

### NEW WAY (Command-Based):
```
1. Create NewAutonomousCommand.h
   (Self-contained ~80 lines)
                │
                ▼
2. Add createNewAutonomous() to CommandFactory
   (3 lines)
                │
                ▼
3. Add WebSocket handler case
   (2 lines: parse, execute)
                │
                ▼
4. Test just NewAutonomousCommand
   (Isolated unit test)
```

## Benefits Visualization

```
┌──────────────────────────────────────────────────────────────┐
│                    OLD SYSTEM                                 │
├──────────────────────────────────────────────────────────────┤
│  WebServer.cpp (600 lines)                                   │
│  ├─ All control modes mixed together                         │
│  ├─ State spread across many variables                       │
│  ├─ Hard to test (requires full hardware)                    │
│  ├─ Changes affect entire file                               │
│  └─ Adding features = rewriting switch statements            │
└──────────────────────────────────────────────────────────────┘

                          ↓ REFACTOR ↓

┌──────────────────────────────────────────────────────────────┐
│                    NEW SYSTEM                                 │
├──────────────────────────────────────────────────────────────┤
│  WebServer.cpp (150 lines)                                   │
│  └─ Thin layer: Parse messages → Execute commands            │
│                                                               │
│  CommandExecutor (100 lines)                                 │
│  └─ Generic: Works with ANY command                          │
│                                                               │
│  JoystickCommand (60 lines)                                  │
│  VelocityCommand (50 lines)                                  │
│  CalibrationCommand (80 lines)                               │
│  AutonomousSequenceCommand (120 lines)                       │
│  └─ Self-contained, testable, reusable                       │
│                                                               │
│  Adding new mode = new 50-100 line file                      │
│  No changes to existing code!                                │
└──────────────────────────────────────────────────────────────┘
```

## Real-World Example: Warehouse Robot

```
┌────────────────────────────────────────────────────────────┐
│  TASK: Pick up box from A, deliver to B                    │
└────────────────────────────────────────────────────────────┘
                            │
                            ▼
            Build autonomous sequence:
                            │
    ┌───────────────────────┼───────────────────────┐
    │                       │                       │
    ▼                       ▼                       ▼
Navigate to A          Pick up box            Navigate to B
    │                       │                       │
    │                       │                       │
    ▼                       ▼                       ▼
┌──────────┐         ┌──────────┐           ┌──────────┐
│DriveToWay│         │LiftArmCmd│           │DriveToWay│
│pointCmd  │────────▶│          │──────────▶│pointCmd  │─┐
│          │         │          │           │          │ │
└──────────┘         └──────────┘           └──────────┘ │
                                                          │
                                                          ▼
                                                   ┌──────────┐
                                                   │LowerArm  │
                                                   │Command   │
                                                   └──────────┘
                                                          │
                                                          ▼
                                                   ┌──────────┐
                                                   │Return    │
                                                   │Home Cmd  │
                                                   └──────────┘

Each command:
  ✓ Self-contained
  ✓ Reports progress
  ✓ Can be interrupted for safety
  ✓ Tested independently
  ✓ Reusable for other tasks
```
