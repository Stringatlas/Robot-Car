#include "WebServer.h"
#include "config.h"
#include "Telemetry.h"
#include "../utils/JsonBuilder.h"

WebServerManager::WebServerManager(int port) 
    : server(port), leftEncoder(nullptr), rightEncoder(nullptr), 
      driveController(nullptr), batteryMonitor(nullptr), velocityController(nullptr), 
      configManager(nullptr), wsHandler(nullptr), controlManager(nullptr),
      commandRouter(nullptr), configHandler(nullptr), httpHandler(nullptr), lastUpdate(0) {}

WebServerManager::~WebServerManager() {
    delete wsHandler;
    delete controlManager;
    delete commandRouter;
    delete configHandler;
    delete httpHandler;
}

void WebServerManager::begin(Encoder* left, Encoder* right, DriveController* drive, 
                             BatteryMonitor* battery, VelocityController* velCtrl, ConfigManager* config) {
    leftEncoder = left;
    rightEncoder = right;
    driveController = drive;
    batteryMonitor = battery;
    velocityController = velCtrl;
    configManager = config;
    
    if (!LittleFS.begin(true)) {
        TELEM_LOG_ERROR("LittleFS Mount Failed");
        return;
    }
    TELEM_LOG_SUCCESS("LittleFS mounted successfully");
    
    initializeComponents();
    setupCallbacks();
    
    wsHandler->begin(&server);
    httpHandler->setupRoutes();
    commandRouter->begin();
    
    server.begin();
    TELEM_LOG_SUCCESS("Web server started");
    TELEM_LOG_INFO("==========");
}

void WebServerManager::initializeComponents() {
    wsHandler = new WebSocketHandler("/ws");
    controlManager = new ClientControlManager();
    
    commandRouter = new WebSocketCommandRouter(
        wsHandler, controlManager, driveController, velocityController, 
        leftEncoder, rightEncoder
    );
    
    configHandler = new ConfigCommandHandler(wsHandler, configManager, velocityController);
    commandRouter->setConfigHandler(configHandler);
    
    httpHandler = new HTTPRouteHandler(
        &server, leftEncoder, rightEncoder, batteryMonitor, velocityController, configManager
    );
    
    Telemetry::getInstance().begin(wsHandler->getWebSocket());
}

void WebServerManager::setupCallbacks() {
    wsHandler->onConnection([this](uint32_t clientId, bool connected) {
        if (connected) {
            String welcome = WebSocketMessageBuilder::buildWelcomeMessage(clientId);
            wsHandler->sendText(clientId, welcome);
            
            auto recentLogs = Telemetry::getInstance().getRecentLogs(20);
            for (const auto& logMsg : recentLogs) {
                wsHandler->sendText(clientId, WebSocketMessageBuilder::buildLogMessage(logMsg));
            }
            
            controlManager->grantControlToFirstClient(clientId);
        } else {
            controlManager->handleClientDisconnect(clientId);
        }
    });
    
    controlManager->onControlStatusChanged([this](uint32_t controllingClientId) {
        broadcastControlStatus();
    });
}

void WebServerManager::broadcastControlStatus() {
    String status = WebSocketMessageBuilder::buildControlStatus(controlManager->getControllingClientId());
    wsHandler->broadcastText(status);
}

void WebServerManager::broadcastEncoderData() {
    if (wsHandler->getClientCount() == 0) return;
    
    float voltage = batteryMonitor->getVoltage();
    
    String json = EncoderJsonBuilder::buildEncoderData(
        leftEncoder->getCount(), leftEncoder->getRevolutions(), leftEncoder->getDistance(), 
        leftEncoder->getVelocity(), leftEncoder->getRPM(),
        rightEncoder->getCount(), rightEncoder->getRevolutions(), rightEncoder->getDistance(),
        rightEncoder->getVelocity(), rightEncoder->getRPM(),
        voltage,
        driveController->getLastLeftPWM(), driveController->getLastRightPWM(),
        velocityController->getLeftVelocityError(), velocityController->getRightVelocityError()
    );
    wsHandler->broadcastText(json);
    
    wsHandler->broadcastText(WebSocketMessageBuilder::buildVelocityError(
        velocityController->getLeftVelocityError(),
        velocityController->getRightVelocityError(),
        velocityController->isPIDEnabled()
    ));
}

void WebServerManager::handleWebSocket() {
    wsHandler->cleanup();
    
    unsigned long now = millis();
    if (now - lastUpdate >= 200) {
        broadcastEncoderData();
        lastUpdate = now;
    }
}

void WebServerManager::update() {
    commandRouter->update();
}
