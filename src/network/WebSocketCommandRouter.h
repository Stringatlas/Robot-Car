#ifndef WEBSOCKETCOMMANDROUTER_H
#define WEBSOCKETCOMMANDROUTER_H

#include <Arduino.h>
#include "WebSocketHandler.h"
#include "ClientControlManager.h"
#include "commands/CommandExecutor.h"
#include "commands/CommandFactory.h"
#include "../drive/DriveController.h"
#include "../drive/VelocityController.h"
#include "../hardware/Encoder.h"

class ConfigCommandHandler;

class WebSocketCommandRouter {
public:
    WebSocketCommandRouter(
        WebSocketHandler* wsHandler,
        ClientControlManager* controlMgr,
        DriveController* drive,
        VelocityController* velCtrl,
        Encoder* leftEnc,
        Encoder* rightEnc
    );
    
    ~WebSocketCommandRouter();
    
    void setConfigHandler(ConfigCommandHandler* handler);
    void begin();
    void update();

private:
    WebSocketHandler* wsHandler;
    ClientControlManager* controlManager;
    DriveController* driveController;
    VelocityController* velocityController;
    Encoder* leftEncoder;
    Encoder* rightEncoder;
    ConfigCommandHandler* configHandler;
    
    CommandExecutor executor;
    CommandFactory* factory;
    
    void handleMessage(uint32_t clientId, const String& message);
    void handleControlCommands(uint32_t clientId, const String& message);
    void handleJoystickCommand(uint32_t clientId, const String& coords);
    void handleMotorCommand(uint32_t clientId, const String& coords);
    void handleVelocityCommand(uint32_t clientId, const String& value);
    void handleCalibrationCommands(uint32_t clientId, const String& message);
    void handlePIDCommands(uint32_t clientId, const String& message);
    void handlePolynomialCommands(uint32_t clientId, const String& message);
    
    bool parseFloatParams(const String& params, float* values, int count);
};

#endif
