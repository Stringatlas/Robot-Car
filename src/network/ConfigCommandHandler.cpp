#include "ConfigCommandHandler.h"
#include "Telemetry.h"

ConfigCommandHandler::ConfigCommandHandler(WebSocketHandler* wsHandler, ConfigManager* configMgr, VelocityController* velCtrl)
    : wsHandler(wsHandler), configManager(configMgr), velocityController(velCtrl) {}

void ConfigCommandHandler::handleConfigCommand(uint32_t clientId, const String& message) {
    if (message == "CONFIG_GET") {
        handleConfigGet(clientId);
    }
    else if (message.startsWith("CONFIG_SET:")) {
        handleConfigSet(clientId, message.substring(11));
    }
    else if (message == "CONFIG_RESET") {
        handleConfigReset(clientId);
    }
}

void ConfigCommandHandler::handleConfigGet(uint32_t clientId) {
    if (configManager) {
        String configJson = configManager->toJson();
        String response = "CONFIG_DATA:" + configJson;
        wsHandler->sendText(clientId, response);
        TELEM_LOG_INFO("Configuration sent to client");
    } else {
        wsHandler->sendText(clientId, "CONFIG_ERROR:ConfigManager not initialized");
    }
}

void ConfigCommandHandler::handleConfigSet(uint32_t clientId, const String& jsonStr) {
    if (!configManager) {
        wsHandler->sendText(clientId, "CONFIG_ERROR:ConfigManager not initialized");
        return;
    }
    
    if (configManager->updateFromJson(jsonStr)) {
        if (configManager->save()) {
            wsHandler->broadcastText("CONFIG_SAVED");
            TELEM_LOG_SUCCESS("Configuration updated and saved");
            
            applyConfigToControllers();
            TELEM_LOG_SUCCESS("Configuration applied to controllers");
        } else {
            wsHandler->sendText(clientId, "CONFIG_ERROR:Failed to save configuration");
        }
    } else {
        wsHandler->sendText(clientId, "CONFIG_ERROR:Failed to parse configuration JSON");
    }
}

void ConfigCommandHandler::handleConfigReset(uint32_t clientId) {
    if (!configManager) {
        wsHandler->sendText(clientId, "CONFIG_ERROR:ConfigManager not initialized");
        return;
    }
    
    configManager->reset();
    if (configManager->save()) {
        wsHandler->broadcastText("CONFIG_RESET");
        TELEM_LOG_SUCCESS("Configuration reset to defaults");
        
        applyConfigToControllers();
    } else {
        wsHandler->sendText(clientId, "CONFIG_ERROR:Failed to save default configuration");
    }
}

void ConfigCommandHandler::applyConfigToControllers() {
    ConfigManager::Config& cfg = configManager->getConfig();
    
    velocityController->setFeedforwardGain(cfg.feedforwardGain);
    velocityController->setDeadzone(cfg.deadzonePWM);
    velocityController->enablePID(cfg.pidEnabled);
    velocityController->setPIDGains(cfg.pidKp, cfg.pidKi, cfg.pidKd);
    velocityController->enablePolynomialMapping(cfg.polynomialEnabled);
    
    float vel2pwm[] = {cfg.vel2pwm_a0, cfg.vel2pwm_a1, cfg.vel2pwm_a2, cfg.vel2pwm_a3};
    float pwm2vel[] = {cfg.pwm2vel_b0, cfg.pwm2vel_b1, cfg.pwm2vel_b2, cfg.pwm2vel_b3};
    velocityController->setVelocityToPWMPolynomial(vel2pwm, 3);
    velocityController->setPWMToVelocityPolynomial(pwm2vel, 3);
}
