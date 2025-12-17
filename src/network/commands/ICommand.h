#ifndef ICOMMAND_H
#define ICOMMAND_H

#include <Arduino.h>

class DriveController;
class VelocityController;
class Encoder;

class ICommand {
public:
    virtual ~ICommand() = default;
    
    virtual bool start() = 0;
    
    virtual bool update() = 0;
    
    virtual void stop() = 0;
    
    virtual bool isBlocking() const = 0;

    virtual const char* getName() const = 0;
    
    virtual bool isInterruptible() const { return true; }
};

#endif
