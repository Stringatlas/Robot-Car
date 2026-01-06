#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "WebSocketHandler.h"
#include "ClientControlManager.h"
#include "WebSocketCommandRouter.h"
#include "ConfigCommandHandler.h"
#include "HTTPRouteHandler.h"
#include "../hardware/Encoder.h"
#include "../drive/DriveController.h"
#include "../hardware/BatteryMonitor.h"
#include "../drive/VelocityController.h"
#include "../utils/ConfigManager.h"

class WebServerManager {
private:
    AsyncWebServer server;
    
    Encoder* leftEncoder;
    Encoder* rightEncoder;
    DriveController* driveController;
    BatteryMonitor* batteryMonitor;
    VelocityController* velocityController;
    ConfigManager* configManager;
    
    WebSocketHandler* wsHandler;
    ClientControlManager* controlManager;
    WebSocketCommandRouter* commandRouter;
    ConfigCommandHandler* configHandler;
    HTTPRouteHandler* httpHandler;
    
    unsigned long lastUpdate;

public:
    WebServerManager(int port);
    ~WebServerManager();
    
    void begin(Encoder* left, Encoder* right, DriveController* drive, 
               BatteryMonitor* battery, VelocityController* velCtrl, ConfigManager* config);
    void handleWebSocket();
    void update();

private:
    void initializeComponents();
    void setupCallbacks();
    void broadcastEncoderData();
    void broadcastControlStatus();
};

#endif
