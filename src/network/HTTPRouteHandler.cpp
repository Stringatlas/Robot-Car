#include "HTTPRouteHandler.h"
#include "Telemetry.h"
#include "../utils/JsonBuilder.h"

HTTPRouteHandler::HTTPRouteHandler(
    AsyncWebServer* server,
    Encoder* leftEnc,
    Encoder* rightEnc,
    BatteryMonitor* battery,
    VelocityController* velCtrl,
    ConfigManager* configMgr
) : server(server), leftEncoder(leftEnc), rightEncoder(rightEnc),
    batteryMonitor(battery), velocityController(velCtrl), configManager(configMgr) {}

void HTTPRouteHandler::setupRoutes() {
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleRoot(request);
    });
    
    server->on("/api/encoders", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleEncoderAPI(request);
    });
    
    server->on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleResetAPI(request);
    });
    
    server->on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleConfigAPI(request);
    });
    
    server->on("/api/config/save", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleConfigSaveAPI(request);
    });
    
    server->on("/upload", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            request->send(200, "text/plain", "File uploaded successfully");
        },
        [this](AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
            handleFileUpload(request, filename, index, data, len, final);
        }
    );
    
    server->serveStatic("/", LittleFS, "/");
}

void HTTPRouteHandler::handleRoot(AsyncWebServerRequest* request) {
    request->send(LittleFS, "/index.html", "text/html; charset=UTF-8");
}

void HTTPRouteHandler::handleEncoderAPI(AsyncWebServerRequest* request) {
    float voltage = batteryMonitor->getVoltage();
    
    String json = EncoderJsonBuilder::buildSimpleEncoderData(
        leftEncoder->getCount(), leftEncoder->getRevolutions(), leftEncoder->getDistance(),
        leftEncoder->getVelocity(), leftEncoder->getRPM(),
        rightEncoder->getCount(), rightEncoder->getRevolutions(), rightEncoder->getDistance(),
        rightEncoder->getVelocity(), rightEncoder->getRPM(),
        voltage
    );
    
    request->send(200, "application/json", json);
}

void HTTPRouteHandler::handleResetAPI(AsyncWebServerRequest* request) {
    leftEncoder->reset();
    rightEncoder->reset();
    request->send(200, "text/plain", "Encoders reset");
}

void HTTPRouteHandler::handleConfigAPI(AsyncWebServerRequest* request) {
    float kp, ki, kd;
    velocityController->getPIDGains(kp, ki, kd);
    
    String json = ConfigJsonBuilder::buildConfigResponse(
        velocityController->getFeedforwardGain(),
        velocityController->getDeadzone(),
        velocityController->isPIDEnabled(),
        kp, ki, kd,
        velocityController->isPolynomialMappingEnabled()
    );
    
    request->send(200, "application/json", json);
}

void HTTPRouteHandler::handleConfigSaveAPI(AsyncWebServerRequest* request) {
    if (!configManager) {
        request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Config manager not initialized\"}");
        return;
    }
    
    auto& cfg = configManager->getConfig();
    cfg.feedforwardGain = velocityController->getFeedforwardGain();
    cfg.deadzonePWM = velocityController->getDeadzone();
    cfg.pidEnabled = velocityController->isPIDEnabled();
    velocityController->getPIDGains(cfg.pidKp, cfg.pidKi, cfg.pidKd);
    cfg.polynomialEnabled = velocityController->isPolynomialMappingEnabled();
    
    if (configManager->save()) {
        request->send(200, "application/json", "{\"status\":\"saved\"}");
        TELEM_LOG_SUCCESS("Configuration saved to file");
    } else {
        request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to save\"}");
    }
}

void HTTPRouteHandler::handleFileUpload(AsyncWebServerRequest* request, String filename, 
                                       size_t index, uint8_t* data, size_t len, bool final) {
    static File uploadFile;
    
    if (!index) {
        Serial.printf("Upload Start: %s\n", filename.c_str());
        String path = "/" + filename;
        uploadFile = LittleFS.open(path, "w");
    }
    
    if (uploadFile) {
        uploadFile.write(data, len);
    }
    
    if (final) {
        if (uploadFile) {
            uploadFile.close();
        }
        Serial.printf("Upload Complete: %s (%u bytes)\n", filename.c_str(), index + len);
    }
}
