#ifndef ICOMMAND_H
#define ICOMMAND_H

#include <Arduino.h>

// Forward declarations
class DriveController;
class VelocityController;
class Encoder;

/**
 * Base interface for all robot commands
 * Supports both blocking (autonomous, calibration) and non-blocking (joystick, manual) commands
 */
class ICommand {
public:
    virtual ~ICommand() = default;
    
    /**
     * Called once when command starts
     * @return true if command started successfully
     */
    virtual bool start() = 0;
    
    /**
     * Called repeatedly in main loop while command is active
     * @return true if command is still running, false when complete
     */
    virtual bool update() = 0;
    
    /**
     * Called when command ends (either completes or is interrupted)
     */
    virtual void stop() = 0;
    
    /**
     * Check if this is a blocking command (prevents other commands from running)
     * @return true for blocking commands (calibration, autonomous), false for non-blocking (joystick)
     */
    virtual bool isBlocking() const = 0;
    
    /**
     * Get command name for logging/debugging
     */
    virtual const char* getName() const = 0;
    
    /**
     * Check if command can be interrupted by new commands
     * @return true if interruptible, false if must complete
     */
    virtual bool isInterruptible() const { return true; }
};

#endif // ICOMMAND_H
