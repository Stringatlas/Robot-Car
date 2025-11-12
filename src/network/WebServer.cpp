#include "WebServer.h"
#include "config.h"
#include "Telemetry.h"
#include "../utils/ConfigManager.h"
#include "../utils/JsonBuilder.h"

WebServerManager::WebServerManager(int port) 
    : server(port), ws("/ws"), leftEncoder(nullptr), rightEncoder(nullptr), driveController(nullptr), batteryMonitor(nullptr), velocityController(nullptr), configManager(nullptr), lastUpdate(0), controllingClientId(0),
      currentMode(ControlMode::IDLE), lastCommandTime(0),
      calibrationActive(false), calibrationMotor(""), calibrationPWM(0), calibrationStartPWM(0), calibrationEndPWM(0), calibrationStepSize(0), calibrationHoldTime(0), calibrationStepStart(0) {}

void WebServerManager::begin(Encoder* left, Encoder* right, DriveController* drive, BatteryMonitor* battery, VelocityController* velCtrl, ConfigManager* config) {
    leftEncoder = left;
    rightEncoder = right;
    driveController = drive;
    batteryMonitor = battery;
    velocityController = velCtrl;
    configManager = config;
    
    // Initialize Telemetry with WebSocket
    Telemetry::getInstance().begin(&ws);
    
    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        TELEM_LOG("ERROR: LittleFS Mount Failed");
        return;
    }
    TELEM_LOG("✓ LittleFS mounted successfully");
    
    // Setup WebSocket
    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, 
                     AwsEventType type, void *arg, uint8_t *data, size_t len) {
        this->onWebSocketEvent(server, client, type, arg, data, len);
    });
    server.addHandler(&ws);
    
    setupRoutes();
    server.begin();
    TELEM_LOG("✓ Web server started");
    TELEM_LOG("✓ WebSocket ready at /ws");
}

void WebServerManager::onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                                       AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        TELEM_LOGF("WebSocket client #%u connected from %s", 
                   client->id(), client->remoteIP().toString().c_str());
        
        // Send the client their ID
        client->text(WebSocketMessageBuilder::buildWelcomeMessage(client->id()));
        
        // Send recent logs to new client
        auto recentLogs = Telemetry::getInstance().getRecentLogs(20);
        for (const auto& logMsg : recentLogs) {
            client->text(WebSocketMessageBuilder::buildLogMessage(logMsg));
        }
        
        // If this is the first client, give them control automatically
        if (controllingClientId == 0) {
            controllingClientId = client->id();
            TELEM_LOGF("Client #%u automatically granted control", client->id());
        }
        broadcastControlStatus();
    } else if (type == WS_EVT_DISCONNECT) {
        TELEM_LOGF("WebSocket client #%u disconnected", client->id());
        // If the controlling client disconnected, release control
        if (controllingClientId == client->id()) {
            controllingClientId = 0;
            TELEM_LOG("Control released (client disconnected)");
            broadcastControlStatus();
        }
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len) {
            // Received complete message
            if (info->opcode == WS_TEXT) {
                data[len] = 0; // Null terminate
                String message = (char*)data;
                
                // Handle commands
                if (message == "RESET") {
                    leftEncoder->reset();
                    rightEncoder->reset();
                    TELEM_LOG("Encoders reset via WebSocket");
                } else if (message == "REQUEST_CONTROL") {
                    controllingClientId = client->id();
                    TELEM_LOGF("Control granted to client #%u", client->id());
                    broadcastControlStatus();
                } else if (message == "RELEASE_CONTROL") {
                    if (controllingClientId == client->id()) {
                        controllingClientId = 0;
                        TELEM_LOGF("Control released by client #%u", client->id());
                        broadcastControlStatus();
                    }
                } else if (message.startsWith("JOYSTICK:")) {
                    // Only process joystick data from the controlling client
                    if (controllingClientId == client->id()) {
                        // Parse joystick data: "JOYSTICK:x,y"
                        String coords = message.substring(9); // Skip "JOYSTICK:"
                        int commaIndex = coords.indexOf(',');
                        if (commaIndex > 0) {
                            float x = coords.substring(0, commaIndex).toFloat();
                            float y = coords.substring(commaIndex + 1).toFloat();
                            // Set mode and control motors: y=forward, x=turn
                            setControlMode(ControlMode::JOYSTICK);
                            driveController->setPowerControl(y, x);
                        }
                    } else {
                        TELEM_LOGF("⚠️ Client #%u tried to send joystick data without control", client->id());
                    }
                } else if (message.startsWith("MOTORS:")) {
                    // Only process direct motor commands from the controlling client
                    if (controllingClientId == client->id()) {
                        // Parse motor data: "MOTORS:left,right"
                        String coords = message.substring(7); // Skip "MOTORS:"
                        int commaIndex = coords.indexOf(',');
                        if (commaIndex > 0) {
                            float leftPower = coords.substring(0, commaIndex).toFloat();
                            float rightPower = coords.substring(commaIndex + 1).toFloat();
                            // Set mode and control motors directly
                            setControlMode(ControlMode::DIRECT_MOTOR);
                            driveController->setLeftMotorPower(leftPower);
                            driveController->setRightMotorPower(rightPower);
                            TELEM_LOGF("🎛️ Direct motor control - L:%.2f R:%.2f", leftPower, rightPower);
                        }
                    } else {
                        TELEM_LOGF("⚠️ Client #%u tried to send motor commands without control", client->id());
                    }
                } else if (message.startsWith("FF_GAIN:")) {
                    if (controllingClientId == client->id()) {
                        float gain = message.substring(8).toFloat();
                        velocityController->setFeedforwardGain(gain);
                    }
                } else if (message.startsWith("DEADZONE:")) {
                    if (controllingClientId == client->id()) {
                        float deadzone = message.substring(9).toFloat();
                        velocityController->setDeadzone(deadzone);
                    }
                } else if (message.startsWith("VELOCITY:")) {
                    if (controllingClientId == client->id()) {
                        float velocity = message.substring(9).toFloat();
                        setControlMode(ControlMode::VELOCITY);
                        velocityController->setVelocity(velocity, velocity);
                        TELEM_LOGF("🎯 Velocity command: %.1f cm/s", velocity);
                        // Acknowledge command back to sender for client-side debugging
                        String ack = WebSocketMessageBuilder::buildCommandAck("VELOCITY", String(velocity, 1));
                        client->text(ack);
                        // Also broadcast to everyone so other clients can see
                        ws.textAll(ack);
                    } else {
                        TELEM_LOGF("⚠️ Client #%u tried to set velocity without control", client->id());
                    }
                } else if (message.startsWith("START_CALIBRATION:")) {
                    // Format: START_CALIBRATION:motor,startPWM,endPWM,stepSize,holdTime
                    String params = message.substring(18);
                    int commaPos1 = params.indexOf(',');
                    int commaPos2 = params.indexOf(',', commaPos1 + 1);
                    int commaPos3 = params.indexOf(',', commaPos2 + 1);
                    int commaPos4 = params.indexOf(',', commaPos3 + 1);
                    
                    calibrationMotor = params.substring(0, commaPos1);
                    calibrationStartPWM = params.substring(commaPos1 + 1, commaPos2).toInt();
                    calibrationEndPWM = params.substring(commaPos2 + 1, commaPos3).toInt();
                    calibrationStepSize = params.substring(commaPos3 + 1, commaPos4).toInt();
                    calibrationHoldTime = params.substring(commaPos4 + 1).toInt();
                    
                    calibrationPWM = calibrationStartPWM;
                    calibrationStepStart = millis();
                    calibrationActive = true;
                    
                    TELEM_LOGF("🔧 Starting calibration: %s motor, %d-%d PWM, step %d, hold %dms", 
                               calibrationMotor.c_str(), calibrationStartPWM, calibrationEndPWM, 
                               calibrationStepSize, calibrationHoldTime);
                } else if (message == "STOP_CALIBRATION") {
                    calibrationActive = false;
                    driveController->setLeftMotorPower(0);
                    driveController->setRightMotorPower(0);
                    TELEM_LOG("⏹️ Calibration stopped");
                } else if (message.startsWith("PID_GAINS:")) {
                    // Format: PID_GAINS:kp,ki,kd
                    String params = message.substring(10);
                    int commaPos1 = params.indexOf(',');
                    int commaPos2 = params.indexOf(',', commaPos1 + 1);
                    
                    float kp = params.substring(0, commaPos1).toFloat();
                    float ki = params.substring(commaPos1 + 1, commaPos2).toFloat();
                    float kd = params.substring(commaPos2 + 1).toFloat();
                    
                    velocityController->setPIDGains(kp, ki, kd);
                } else if (message.startsWith("PID_ENABLE:")) {
                    bool enable = message.substring(11) == "true";
                    velocityController->enablePID(enable);
                    
                } else if (message.startsWith("POLY_VEL2PWM:")) {
                    // Format: POLY_VEL2PWM:degree,a0,a1,a2,...
                    // Example cubic: POLY_VEL2PWM:3,60.5,0.2,0.015,0.0003
                    String params = message.substring(13);
                    int commaPos = params.indexOf(',');
                    if (commaPos > 0) {
                        int degree = params.substring(0, commaPos).toInt();
                        params = params.substring(commaPos + 1);
                        
                        float coeffs[6] = {0};  // Support up to degree 5
                        int idx = 0;
                        while (params.length() > 0 && idx <= degree && idx < 6) {
                            commaPos = params.indexOf(',');
                            if (commaPos > 0) {
                                coeffs[idx++] = params.substring(0, commaPos).toFloat();
                                params = params.substring(commaPos + 1);
                            } else {
                                coeffs[idx++] = params.toFloat();
                                break;
                            }
                        }
                        
                        velocityController->setVelocityToPWMPolynomial(coeffs, degree);
                        ws.textAll(WebSocketMessageBuilder::buildCommandAck("POLY_VEL2PWM", "degree=" + String(degree)));
                    }
                    
                } else if (message.startsWith("POLY_PWM2VEL:")) {
                    // Format: POLY_PWM2VEL:degree,a0,a1,a2,...
                    // Example cubic: POLY_PWM2VEL:3,-20.5,0.15,0.0008,0.000002
                    String params = message.substring(13);
                    int commaPos = params.indexOf(',');
                    if (commaPos > 0) {
                        int degree = params.substring(0, commaPos).toInt();
                        params = params.substring(commaPos + 1);
                        
                        float coeffs[6] = {0};
                        int idx = 0;
                        while (params.length() > 0 && idx <= degree && idx < 6) {
                            commaPos = params.indexOf(',');
                            if (commaPos > 0) {
                                coeffs[idx++] = params.substring(0, commaPos).toFloat();
                                params = params.substring(commaPos + 1);
                            } else {
                                coeffs[idx++] = params.toFloat();
                                break;
                            }
                        }
                        
                        velocityController->setPWMToVelocityPolynomial(coeffs, degree);
                        ws.textAll(WebSocketMessageBuilder::buildCommandAck("POLY_PWM2VEL", "degree=" + String(degree)));
                    }
                    
                } else if (message.startsWith("POLY_ENABLE:")) {
                    bool enable = message.substring(12) == "true";
                    velocityController->enablePolynomialMapping(enable);
                    ws.textAll(WebSocketMessageBuilder::buildCommandAck("POLY_ENABLE", enable ? "true" : "false"));
                    
                } else if (message == "CONFIG_GET") {
                    // Send current configuration to requesting client
                    if (configManager) {
                        String configJson = configManager->toJson();
                        String response = "CONFIG_DATA:" + configJson;
                        client->text(response);
                        TELEM_LOG("Configuration sent to client");
                    } else {
                        client->text("CONFIG_ERROR:ConfigManager not initialized");
                    }
                    
                } else if (message.startsWith("CONFIG_SET:")) {
                    // Update configuration from JSON
                    if (configManager) {
                        String jsonStr = message.substring(11);
                        if (configManager->updateFromJson(jsonStr)) {
                            if (configManager->save()) {
                                ws.textAll("CONFIG_SAVED");
                                TELEM_LOG("✓ Configuration updated and saved");
                                
                                // Apply the new configuration to velocity controller
                                ConfigManager::Config& cfg = configManager->getConfig();
                                velocityController->setFeedforwardGain(cfg.feedforwardGain);
                                velocityController->setDeadzone(cfg.deadzonePWM);
                                velocityController->enablePID(cfg.pidEnabled);
                                velocityController->setPIDGains(cfg.pidKp, cfg.pidKi, cfg.pidKd);
                                velocityController->enablePolynomialMapping(cfg.polynomialEnabled);
                                
                                // Set polynomial coefficients
                                float vel2pwm[] = {cfg.vel2pwm_a0, cfg.vel2pwm_a1, cfg.vel2pwm_a2, cfg.vel2pwm_a3};
                                float pwm2vel[] = {cfg.pwm2vel_b0, cfg.pwm2vel_b1, cfg.pwm2vel_b2, cfg.pwm2vel_b3};
                                velocityController->setVelocityToPWMPolynomial(vel2pwm, 3);
                                velocityController->setPWMToVelocityPolynomial(pwm2vel, 3);
                                
                                TELEM_LOG("✓ Configuration applied to controllers");
                            } else {
                                client->text("CONFIG_ERROR:Failed to save configuration");
                            }
                        } else {
                            client->text("CONFIG_ERROR:Failed to parse configuration JSON");
                        }
                    } else {
                        client->text("CONFIG_ERROR:ConfigManager not initialized");
                    }
                    
                } else if (message == "CONFIG_RESET") {
                    // Reset configuration to defaults
                    if (configManager) {
                        configManager->reset();
                        if (configManager->save()) {
                            ws.textAll("CONFIG_RESET");
                            TELEM_LOG("✓ Configuration reset to defaults");
                            
                            // Apply default configuration to velocity controller
                            ConfigManager::Config& cfg = configManager->getConfig();
                            velocityController->setFeedforwardGain(cfg.feedforwardGain);
                            velocityController->setDeadzone(cfg.deadzonePWM);
                            velocityController->enablePID(cfg.pidEnabled);
                            velocityController->setPIDGains(cfg.pidKp, cfg.pidKi, cfg.pidKd);
                            velocityController->enablePolynomialMapping(cfg.polynomialEnabled);
                        } else {
                            client->text("CONFIG_ERROR:Failed to save default configuration");
                        }
                    } else {
                        client->text("CONFIG_ERROR:ConfigManager not initialized");
                    }
                }
            }
        }
    }
}

void WebServerManager::broadcastControlStatus() {
    ws.textAll(WebSocketMessageBuilder::buildControlStatus(controllingClientId));
}

void WebServerManager::broadcastEncoderData() {
    // Only send if there are connected clients
    if (ws.count() == 0) return;
    
    // Read battery voltage
    float voltage = batteryMonitor->getVoltage();
    
    // Build and broadcast encoder JSON using helper
    String json = EncoderJsonBuilder::buildEncoderData(
        leftEncoder->getCount(), leftEncoder->getRevolutions(), leftEncoder->getDistance(), 
        leftEncoder->getVelocity(), leftEncoder->getRPM(),
        rightEncoder->getCount(), rightEncoder->getRevolutions(), rightEncoder->getDistance(),
        rightEncoder->getVelocity(), rightEncoder->getRPM(),
        voltage,
        driveController->getLastLeftPWM(), driveController->getLastRightPWM(),
        velocityController->getLeftVelocityError(), velocityController->getRightVelocityError()
    );
    ws.textAll(json);
    
    // Also send velocity error message for calibration page
    ws.textAll(WebSocketMessageBuilder::buildVelocityError(
        velocityController->getLeftVelocityError(),
        velocityController->getRightVelocityError(),
        velocityController->isPIDEnabled()
    ));
}

void WebServerManager::handleWebSocket() {
    // Clean up disconnected clients
    ws.cleanupClients();
    
    // Send data every 200ms
    unsigned long now = millis();
    if (now - lastUpdate >= 200) {
        broadcastEncoderData();
        lastUpdate = now;
    }
}

void WebServerManager::setupRoutes() {
    // Serve index.html from LittleFS with UTF-8 encoding
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        request->send(LittleFS, "/index.html", "text/html; charset=UTF-8");
    });
    
    // API endpoint to get encoder data as JSON
    server.on("/api/encoders", HTTP_GET, [this](AsyncWebServerRequest *request){
        // Read battery voltage
        float voltage = batteryMonitor->getVoltage();
        
        String json = EncoderJsonBuilder::buildSimpleEncoderData(
            leftEncoder->getCount(), leftEncoder->getRevolutions(), leftEncoder->getDistance(),
            leftEncoder->getVelocity(), leftEncoder->getRPM(),
            rightEncoder->getCount(), rightEncoder->getRevolutions(), rightEncoder->getDistance(),
            rightEncoder->getVelocity(), rightEncoder->getRPM(),
            voltage
        );
        
        request->send(200, "application/json", json);
    });
    
    // Reset encoders endpoint
    server.on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest *request){
        leftEncoder->reset();
        rightEncoder->reset();
        request->send(200, "text/plain", "Encoders reset");
    });
    
    // Configuration endpoints
    server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest *request){
        // Return current configuration as JSON
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
    });
    
    server.on("/api/config/save", HTTP_POST, [this](AsyncWebServerRequest *request){
        // Save current settings to LittleFS config file
        if (configManager) {
            // Update config with current controller values
            auto& cfg = configManager->getConfig();
            cfg.feedforwardGain = velocityController->getFeedforwardGain();
            cfg.deadzonePWM = velocityController->getDeadzone();
            cfg.pidEnabled = velocityController->isPIDEnabled();
            velocityController->getPIDGains(cfg.pidKp, cfg.pidKi, cfg.pidKd);
            cfg.polynomialEnabled = velocityController->isPolynomialMappingEnabled();
            
            // Save to file
            if (configManager->save()) {
                request->send(200, "application/json", "{\"status\":\"saved\"}");
                TELEM_LOG("✓ Configuration saved to file");
            } else {
                request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to save\"}");
            }
        } else {
            request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Config manager not initialized\"}");
        }
    });
    
    // File upload endpoint for OTA web file updates
    server.on("/upload", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            request->send(200, "text/plain", "File uploaded successfully");
        },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
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
    );
    
    // Serve static files (CSS, JS, etc.)
    server.serveStatic("/", LittleFS, "/");
}

void WebServerManager::updateCalibration() {
    if (!calibrationActive) return;
    
    unsigned long now = millis();
    
    // Check if we've held this PWM value long enough
    if (now - calibrationStepStart >= calibrationHoldTime) {
        // Read current velocities
        float leftVel = leftEncoder->getVelocity();
        float rightVel = rightEncoder->getVelocity();
        
        // Send data point to client
        ws.textAll(WebSocketMessageBuilder::buildCalibrationPoint(calibrationPWM, leftVel, rightVel));
        
        // Progress update
        ws.textAll(WebSocketMessageBuilder::buildCalibrationProgress(calibrationPWM, calibrationEndPWM, calibrationStartPWM));
        
        // Move to next PWM value
        calibrationPWM += calibrationStepSize;
        
        if (calibrationPWM > calibrationEndPWM) {
            // Calibration complete
            calibrationActive = false;
            driveController->setLeftMotorPower(0);
            driveController->setRightMotorPower(0);
            ws.textAll("CALIBRATION_COMPLETE");
            TELEM_LOG("✓ Calibration complete");
        } else {
            // Set new PWM value
            float pwmValue = calibrationPWM / 255.0;  // Convert to 0-1 range
            
            if (calibrationMotor == "left") {
                driveController->setLeftMotorPower(pwmValue);
                driveController->setRightMotorPower(0);
            } else if (calibrationMotor == "right") {
                driveController->setLeftMotorPower(0);
                driveController->setRightMotorPower(pwmValue);
            } else if (calibrationMotor == "both") {
                driveController->setLeftMotorPower(pwmValue);
                driveController->setRightMotorPower(pwmValue);
            }
            
            calibrationStepStart = now;
        }
    }
}

void WebServerManager::setControlMode(ControlMode mode) {
    if (currentMode != mode) {
        // Stop everything when switching modes
        if (mode == ControlMode::IDLE) {
            driveController->setLeftMotorPower(0);
            driveController->setRightMotorPower(0);
            velocityController->setVelocity(0, 0);
        }
        
        currentMode = mode;
        lastCommandTime = millis();
        
        const char* modeNames[] = {"IDLE", "JOYSTICK", "DIRECT_MOTOR", "VELOCITY", "CALIBRATION"};
        TELEM_LOGF("🔄 Control mode: %s", modeNames[(int)mode]);
    }
}

void WebServerManager::checkControlTimeout() {
    // If no commands received recently, revert to IDLE (except during calibration)
    if (currentMode != ControlMode::IDLE && 
        currentMode != ControlMode::CALIBRATION && 
        millis() - lastCommandTime > CONTROL_TIMEOUT_MS) {
        setControlMode(ControlMode::IDLE);
    }
}

void WebServerManager::update() {
    // Check if control has timed out
    checkControlTimeout();
    
    // Update calibration if active
    if (calibrationActive) {
        setControlMode(ControlMode::CALIBRATION);
        updateCalibration();
    }
    
    // Apply control based on current mode
    switch (currentMode) {
        case ControlMode::IDLE:
            // Do nothing - motors stay at last commanded state or zero
            break;
            
        case ControlMode::JOYSTICK:
            // Joystick commands are applied directly in WebSocket handler
            // Just need to update velocity controller in case it's enabled
            break;
            
        case ControlMode::DIRECT_MOTOR:
            // Direct motor commands are applied in WebSocket handler
            // Nothing to do here
            break;
            
        case ControlMode::VELOCITY:
            // Let velocity controller take over
            velocityController->update();
            break;
            
        case ControlMode::CALIBRATION:
            // Calibration is handled in updateCalibration()
            // Don't run velocity controller during calibration
            break;
    }
}
