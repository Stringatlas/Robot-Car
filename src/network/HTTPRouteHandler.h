#ifndef HTTPROUTEHANDLER_H
#define HTTPROUTEHANDLER_H

#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "../hardware/Encoder.h"
#include "../hardware/BatteryMonitor.h"
#include "../drive/VelocityController.h"
#include "../utils/ConfigManager.h"

class HTTPRouteHandler {
public:
    HTTPRouteHandler(
        AsyncWebServer* server,
        Encoder* leftEnc,
        Encoder* rightEnc,
        BatteryMonitor* battery,
        VelocityController* velCtrl,
        ConfigManager* configMgr
    );
    
    void setupRoutes();

private:
    AsyncWebServer* server;
    Encoder* leftEncoder;
    Encoder* rightEncoder;
    BatteryMonitor* batteryMonitor;
    VelocityController* velocityController;
    ConfigManager* configManager;
    
    void handleRoot(AsyncWebServerRequest* request);
    void handleEncoderAPI(AsyncWebServerRequest* request);
    void handleResetAPI(AsyncWebServerRequest* request);
    void handleConfigAPI(AsyncWebServerRequest* request);
    void handleConfigSaveAPI(AsyncWebServerRequest* request);
    void handleFileUpload(AsyncWebServerRequest* request, String filename, 
                         size_t index, uint8_t* data, size_t len, bool final);
};

#endif
