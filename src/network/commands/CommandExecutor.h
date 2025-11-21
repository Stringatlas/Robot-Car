#ifndef COMMAND_EXECUTOR_H
#define COMMAND_EXECUTOR_H

#include "ICommand.h"
#include <memory>
#include "../Telemetry.h"

/**
 * Manages command execution and lifecycle
 * Handles blocking vs non-blocking commands, interruption, and state transitions
 */
class CommandExecutor {
private:
    std::unique_ptr<ICommand> currentCommand;
    bool commandRunning;

public:
    CommandExecutor() : commandRunning(false) {}
    
    /**
     * Execute a new command
     * - Non-blocking commands can interrupt other non-blocking commands
     * - Blocking commands stop any running command first
     * - If current command is non-interruptible, request is rejected
     * 
     * @param command The command to execute (executor takes ownership)
     * @return true if command started successfully
     */
    bool executeCommand(std::unique_ptr<ICommand> command) {
        if (!command) return false;
        
        // Check if we can interrupt current command
        if (currentCommand) {
            if (!currentCommand->isInterruptible()) {
                // Current command cannot be interrupted
                TELEM_LOGF("⚠️ Cannot interrupt non-interruptible command: %s", 
                          currentCommand->getName());
                return false;
            }
            
            // Stop current command
            TELEM_LOGF("🛑 Stopping command: %s", currentCommand->getName());
            currentCommand->stop();
            currentCommand.reset();
            commandRunning = false;
        }
        
        // Start new command
        if (command->start()) {
            TELEM_LOGF("▶️ Started command: %s (%s)", 
                      command->getName(),
                      command->isBlocking() ? "blocking" : "non-blocking");
            currentCommand = std::move(command);
            commandRunning = true;
            return true;
        }
        
        TELEM_LOGF("❌ Failed to start command: %s", command->getName());
        return false;
    }
    
    /**
     * Update current command (call in main loop)
     * Returns false when command completes, true while running
     */
    void update() {
        if (!commandRunning || !currentCommand) {
            return;
        }
        
        // Update current command
        bool stillRunning = currentCommand->update();
        
        if (!stillRunning) {
            // Command completed
            TELEM_LOGF("✓ Command completed: %s", currentCommand->getName());
            currentCommand->stop();
            currentCommand.reset();
            commandRunning = false;
        }
    }
    
    /**
     * Stop current command immediately
     */
    void stopCurrentCommand() {
        if (currentCommand) {
            TELEM_LOGF("🛑 Stopping command: %s", currentCommand->getName());
            currentCommand->stop();
            currentCommand.reset();
            commandRunning = false;
        }
    }
    
    /**
     * Check if a command is currently running
     */
    bool isCommandRunning() const {
        return commandRunning && currentCommand != nullptr;
    }
    
    /**
     * Check if current command is blocking
     */
    bool isBlockingCommandRunning() const {
        return commandRunning && currentCommand && currentCommand->isBlocking();
    }
    
    /**
     * Get name of current command (or nullptr if none)
     */
    const char* getCurrentCommandName() const {
        return currentCommand ? currentCommand->getName() : nullptr;
    }
    
    /**
     * Get raw pointer to current command for type-specific operations
     * Use carefully - ownership remains with executor
     */
    template<typename T>
    T* getCurrentCommandAs() {
        return dynamic_cast<T*>(currentCommand.get());
    }
};

#endif // COMMAND_EXECUTOR_H
