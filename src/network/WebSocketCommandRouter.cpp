#include "WebSocketCommandRouter.h"
#include "ConfigCommandHandler.h"
#include "Telemetry.h"
#include "../utils/JsonBuilder.h"
#include "commands/JoystickCommand.h"
#include "commands/DirectMotorCommand.h"
#include "commands/VelocityCommand.h"
#include "commands/CalibrationCommand.h"

WebSocketCommandRouter::WebSocketCommandRouter(
    WebSocketHandler* wsHandler,
    ClientControlManager* controlMgr,
    DriveController* drive,
    VelocityController* velCtrl,
    Encoder* leftEnc,
    Encoder* rightEnc
) : wsHandler(wsHandler), controlManager(controlMgr),
    driveController(drive), velocityController(velCtrl),
    leftEncoder(leftEnc), rightEncoder(rightEnc), configHandler(nullptr) {
    factory = new CommandFactory(drive, velCtrl, leftEnc, rightEnc);
}

WebSocketCommandRouter::~WebSocketCommandRouter() {
    delete factory;
}

void WebSocketCommandRouter::setConfigHandler(ConfigCommandHandler* handler) {
    configHandler = handler;
}

void WebSocketCommandRouter::begin() {
    wsHandler->onMessage([this](uint32_t clientId, const String& message) {
        handleMessage(clientId, message);
    });
}

void WebSocketCommandRouter::update() {
    executor.update();
}

void WebSocketCommandRouter::handleMessage(uint32_t clientId, const String& message) {
    if (message == "RESET") {
        leftEncoder->reset();
        rightEncoder->reset();
        TELEM_LOG_COMMAND("Encoders reset via WebSocket");
    } 
    else if (message == "REQUEST_CONTROL") {
        controlManager->requestControl(clientId);
    } 
    else if (message == "RELEASE_CONTROL") {
        controlManager->releaseControl(clientId);
    }
    else if (message.startsWith("JOYSTICK:")) {
        handleJoystickCommand(clientId, message.substring(9));
    }
    else if (message.startsWith("MOTORS:")) {
        handleMotorCommand(clientId, message.substring(7));
    }
    else if (message.startsWith("VELOCITY:")) {
        handleVelocityCommand(clientId, message.substring(9));
    }
    else if (message.startsWith("FF_GAIN:")) {
        if (controlManager->hasControl(clientId)) {
            float gain = message.substring(8).toFloat();
            velocityController->setFeedforwardGain(gain);
        }
    }
    else if (message.startsWith("DEADZONE:")) {
        if (controlManager->hasControl(clientId)) {
            float deadzone = message.substring(9).toFloat();
            velocityController->setDeadzone(deadzone);
        }
    }
    else if (message.startsWith("START_CALIBRATION:") || message == "STOP_CALIBRATION") {
        handleCalibrationCommands(clientId, message);
    }
    else if (message.startsWith("PID_")) {
        handlePIDCommands(clientId, message);
    }
    else if (message.startsWith("POLY_")) {
        handlePolynomialCommands(clientId, message);
    }
    else if (message.startsWith("CONFIG_") && configHandler) {
        configHandler->handleConfigCommand(clientId, message);
    }
}

void WebSocketCommandRouter::handleJoystickCommand(uint32_t clientId, const String& coords) {
    if (!controlManager->hasControl(clientId)) {
        TELEM_LOGF_WARNING("Client #%u tried to send joystick data without control", clientId);
        return;
    }
    
    int commaIndex = coords.indexOf(',');
    if (commaIndex > 0) {
        float x = coords.substring(0, commaIndex).toFloat();
        float y = coords.substring(commaIndex + 1).toFloat();
        
        JoystickCommand* cmd = executor.getCurrentCommandAs<JoystickCommand>();
        if (cmd) {
            cmd->updateJoystick(x, y);
        } else {
            auto newCmd = factory->createJoystickCommand();
            newCmd->updateJoystick(x, y);
            executor.executeCommand(std::move(newCmd));
        }
    }
}

void WebSocketCommandRouter::handleMotorCommand(uint32_t clientId, const String& coords) {
    if (!controlManager->hasControl(clientId)) {
        TELEM_LOGF_WARNING("Client #%u tried to send motor commands without control", clientId);
        return;
    }
    
    int commaIndex = coords.indexOf(',');
    if (commaIndex > 0) {
        float leftPower = coords.substring(0, commaIndex).toFloat();
        float rightPower = coords.substring(commaIndex + 1).toFloat();
        
        DirectMotorCommand* cmd = executor.getCurrentCommandAs<DirectMotorCommand>();
        if (cmd) {
            cmd->setMotorPowers(leftPower, rightPower);
        } else {
            auto newCmd = factory->createDirectMotorCommand(leftPower, rightPower);
            executor.executeCommand(std::move(newCmd));
        }
        
        TELEM_LOGF_COMMAND("Direct motor control - L:%.2f R:%.2f", leftPower, rightPower);
    }
}

void WebSocketCommandRouter::handleVelocityCommand(uint32_t clientId, const String& value) {
    if (!controlManager->hasControl(clientId)) {
        TELEM_LOGF_WARNING("Client #%u tried to set velocity without control", clientId);
        return;
    }
    
    float velocity = value.toFloat();
    
    VelocityCommand* cmd = executor.getCurrentCommandAs<VelocityCommand>();
    if (cmd) {
        cmd->updateVelocity(velocity);
    } else {
        executor.executeCommand(factory->createVelocityCommand(velocity));
    }
    
    TELEM_LOGF_COMMAND("Velocity command: %.1f cm/s", velocity);
    
    String ack = WebSocketMessageBuilder::buildCommandAck("VELOCITY", String(velocity, 1));
    wsHandler->sendText(clientId, ack);
    wsHandler->broadcastText(ack);
}

void WebSocketCommandRouter::handleCalibrationCommands(uint32_t clientId, const String& message) {
    if (message.startsWith("START_CALIBRATION:")) {
        String params = message.substring(18);
        int commaPos1 = params.indexOf(',');
        int commaPos2 = params.indexOf(',', commaPos1 + 1);
        int commaPos3 = params.indexOf(',', commaPos2 + 1);
        int commaPos4 = params.indexOf(',', commaPos3 + 1);
        
        CalibrationCommand::Config config;
        config.motor = params.substring(0, commaPos1);
        config.startPWM = params.substring(commaPos1 + 1, commaPos2).toInt();
        config.endPWM = params.substring(commaPos2 + 1, commaPos3).toInt();
        config.stepSize = params.substring(commaPos3 + 1, commaPos4).toInt();
        config.holdTime = params.substring(commaPos4 + 1).toInt();
        
        auto cmd = factory->createCalibrationCommand(config);
        
        cmd->setDataCallback([this](const CalibrationCommand::DataPoint& point) {
            wsHandler->broadcastText(WebSocketMessageBuilder::buildCalibrationPoint(
                point.pwm, point.leftVelocity, point.rightVelocity));
        });
        
        cmd->setProgressCallback([this](int current, int end, int start) {
            wsHandler->broadcastText(WebSocketMessageBuilder::buildCalibrationProgress(
                current, end, start));
        });
        
        cmd->setCompleteCallback([this]() {
            wsHandler->broadcastText("CALIBRATION_COMPLETE");
        });
        
        executor.executeCommand(std::move(cmd));
    } 
    else if (message == "STOP_CALIBRATION") {
        executor.stopCurrentCommand();
    }
}

void WebSocketCommandRouter::handlePIDCommands(uint32_t clientId, const String& message) {
    if (message.startsWith("PID_GAINS:")) {
        String params = message.substring(10);
        int commaPos1 = params.indexOf(',');
        int commaPos2 = params.indexOf(',', commaPos1 + 1);
        
        float kp = params.substring(0, commaPos1).toFloat();
        float ki = params.substring(commaPos1 + 1, commaPos2).toFloat();
        float kd = params.substring(commaPos2 + 1).toFloat();
        
        velocityController->setPIDGains(kp, ki, kd);
    }
    else if (message.startsWith("PID_ENABLE:")) {
        bool enable = message.substring(11) == "true";
        velocityController->enablePID(enable);
    }
}

void WebSocketCommandRouter::handlePolynomialCommands(uint32_t clientId, const String& message) {
    if (message.startsWith("POLY_VEL2PWM:")) {
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
            
            velocityController->setVelocityToPWMPolynomial(coeffs, degree);
            wsHandler->broadcastText(WebSocketMessageBuilder::buildCommandAck("POLY_VEL2PWM", "degree=" + String(degree)));
        }
    }
    else if (message.startsWith("POLY_PWM2VEL:")) {
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
            wsHandler->broadcastText(WebSocketMessageBuilder::buildCommandAck("POLY_PWM2VEL", "degree=" + String(degree)));
        }
    }
    else if (message.startsWith("POLY_ENABLE:")) {
        bool enable = message.substring(12) == "true";
        velocityController->enablePolynomialMapping(enable);
        wsHandler->broadcastText(WebSocketMessageBuilder::buildCommandAck("POLY_ENABLE", enable ? "true" : "false"));
    }
}
