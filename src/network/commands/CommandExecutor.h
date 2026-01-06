#ifndef COMMAND_EXECUTOR_H
#define COMMAND_EXECUTOR_H

#include "ICommand.h"
#include <memory>
#include "../Telemetry.h"

class CommandExecutor {
private:
    std::unique_ptr<ICommand> currentCommand;
    bool commandRunning;

public:
    CommandExecutor() : commandRunning(false) {}
    
    bool executeCommand(std::unique_ptr<ICommand> command) {
        if (!command) return false;
        
        if (currentCommand) {
            if (!currentCommand->isInterruptible()) {
                TELEM_LOGF("⚠️ Cannot interrupt non-interruptible command: %s", 
                          currentCommand->getName());
                return false;
            }
            
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
    
    void update() {
        if (!commandRunning || !currentCommand) {
            return;
        }
        
        bool stillRunning = currentCommand->update();
        
        if (!stillRunning) {
            TELEM_LOGF_SUCCESS("Command completed: %s", currentCommand->getName());
            currentCommand->stop();
            currentCommand.reset();
            commandRunning = false;
        }
    }
    
    void stopCurrentCommand() {
        if (currentCommand) {
            TELEM_LOGF_ERROR("Stopping command: %s", currentCommand->getName());
            currentCommand->stop();
            currentCommand.reset();
            commandRunning = false;
        }
    }

    bool isCommandRunning() const {
        return commandRunning && currentCommand != nullptr;
    }
    
    bool isBlockingCommandRunning() const {
        return commandRunning && currentCommand && currentCommand->isBlocking();
    }
    
    const char* getCurrentCommandName() const {
        return currentCommand ? currentCommand->getName() : nullptr;
    }
    
    template<typename T>
    T* getCurrentCommandAs() {
        return dynamic_cast<T*>(currentCommand.get());
    }
};

#endif
