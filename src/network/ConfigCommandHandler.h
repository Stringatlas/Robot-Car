#ifndef CONFIGCOMMANDHANDLER_H
#define CONFIGCOMMANDHANDLER_H

#include <Arduino.h>
#include "WebSocketHandler.h"
#include "../utils/ConfigManager.h"
#include "../drive/VelocityController.h"

class ConfigCommandHandler {
public:
    ConfigCommandHandler(WebSocketHandler* wsHandler, ConfigManager* configMgr, VelocityController* velCtrl);
    
    void handleConfigCommand(uint32_t clientId, const String& message);

private:
    WebSocketHandler* wsHandler;
    ConfigManager* configManager;
    VelocityController* velocityController;
    
    void handleConfigGet(uint32_t clientId);
    void handleConfigSet(uint32_t clientId, const String& jsonStr);
    void handleConfigReset(uint32_t clientId);
    void applyConfigToControllers();
};

#endif
