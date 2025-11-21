#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include "config.h"
#include "hardware/Encoder.h"
#include "hardware/IMU.h"
#include "network/WebServer.h"
#include "drive/DriveController.h"
#include "hardware/BatteryMonitor.h"
#include "drive/VelocityController.h"
#include "network/Telemetry.h"
#include "utils/ConfigManager.h"

Encoder leftEncoder(LEFT_ENCODER_A, LEFT_ENCODER_B, ENCODER_PPR, WHEEL_DIAMETER);
Encoder rightEncoder(RIGHT_ENCODER_A, RIGHT_ENCODER_B, ENCODER_PPR, WHEEL_DIAMETER, true); // Reversed
DriveController driveController;
BatteryMonitor batteryMonitor(BATTERY_VOLTAGE_PIN, BATTERY_VOLTAGE_MULTIPLIER);
VelocityController velocityController;
ConfigManager configManager;
WebServerManager webServer(WEB_SERVER_PORT);
IMU imu;

unsigned long lastIMULog = 0;

void setupWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    Serial.print("Connecting to WiFi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        TELEM_LOG("✓ WiFi Connected!");
        TELEM_LOGF("IP Address: %s", WiFi.localIP().toString().c_str());
        TELEM_LOGF("Web Interface: http://%s", WiFi.localIP().toString().c_str());
    } else {
        Serial.println();
        TELEM_LOG("✗ WiFi connection failed!");
    }
}

void setupOTA() {
    // Setup mDNS
    if (!MDNS.begin(OTA_HOSTNAME)) {
        TELEM_LOG("✗ Error setting up MDNS responder!");
        return;
    }
    TELEM_LOGF("✓ mDNS responder started: %s.local", OTA_HOSTNAME);
    
    // Port defaults to 3232
    // ArduinoOTA.setPort(3232);
    
    // Hostname defaults to esp3232-[MAC]
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    
    // Authentication with password
    ArduinoOTA.setPassword(OTA_PASSWORD);
    
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else {  // U_SPIFFS
            type = "filesystem";
        }
        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        TELEM_LOGF("[OTA] Start updating %s", type.c_str());
    });
    
    ArduinoOTA.onEnd([]() {
        TELEM_LOG("[OTA] Update complete!");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        String errorMsg = "[OTA] Error: ";
        if (error == OTA_AUTH_ERROR) {
            errorMsg += "Auth Failed";
        } else if (error == OTA_BEGIN_ERROR) {
            errorMsg += "Begin Failed";
        } else if (error == OTA_CONNECT_ERROR) {
            errorMsg += "Connect Failed";
        } else if (error == OTA_RECEIVE_ERROR) {
            errorMsg += "Receive Failed";
        } else if (error == OTA_END_ERROR) {
            errorMsg += "End Failed";
        }
        TELEM_LOG(errorMsg);
    });
    
    ArduinoOTA.begin();
    TELEM_LOG("✓ OTA ready");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n=== ESP32 Robot Car ===");
    Serial.println("Initializing...\n");
    
    // Initialize battery monitor
    batteryMonitor.begin();
    
    // Initialize drive controller
    driveController.begin();
    
    // Register encoder instances (required for ISR callbacks)
    Encoder::registerLeft(&leftEncoder);
    Encoder::registerRight(&rightEncoder);
    
    // Initialize encoders
    TELEM_LOG("Setting up encoders...");
    leftEncoder.begin();
    rightEncoder.begin();
    TELEM_LOG("✓ Encoders initialized");
    
    // Initialize IMU
    TELEM_LOG("Setting up IMU...");
    if (imu.begin()) {
        TELEM_LOG("✓ IMU connected");
        TELEM_LOG("Calibrating IMU (keep still)...");
        imu.calibrate();
        TELEM_LOG("✓ IMU calibrated");
    } else {
        TELEM_LOG("✗ IMU connection failed!");
    }
    
    // Initialize velocity controller
    velocityController.attachEncoders(&leftEncoder, &rightEncoder);
    velocityController.begin();
    
    // Load saved configuration and apply to velocity controller
    TELEM_LOG("Loading configuration...");
    if (configManager.load()) {
        auto& cfg = configManager.getConfig();
        
        // Apply velocity controller parameters
        velocityController.setFeedforwardGain(cfg.feedforwardGain);
        velocityController.setDeadzone(cfg.deadzonePWM);
        velocityController.setPIDGains(cfg.pidKp, cfg.pidKi, cfg.pidKd);
        velocityController.enablePID(cfg.pidEnabled);
        
        // Apply polynomial coefficients if enabled
        if (cfg.polynomialEnabled) {
            float vel2pwm[] = {cfg.vel2pwm_a0, cfg.vel2pwm_a1, cfg.vel2pwm_a2, cfg.vel2pwm_a3};
            float pwm2vel[] = {cfg.pwm2vel_b0, cfg.pwm2vel_b1, cfg.pwm2vel_b2, cfg.pwm2vel_b3};
            velocityController.setVelocityToPWMPolynomial(vel2pwm, 3);
            velocityController.setPWMToVelocityPolynomial(pwm2vel, 3);
            velocityController.enablePolynomialMapping(true);
        }
        
        configManager.print();
    } else {
        TELEM_LOG("No saved configuration found - using defaults");
    }
    
    // Setup WiFi (Station Mode)
    setupWiFi();
    
    // Setup OTA
    if (WiFi.status() == WL_CONNECTED) {
        setupOTA();
    }
    
    // Setup Web Server (this also initializes Telemetry)
    webServer.begin(&leftEncoder, &rightEncoder, &driveController, &batteryMonitor, &velocityController, &configManager);
    
    TELEM_LOG("=== System Ready ===");
}

void loop() {
    ArduinoOTA.handle();

    webServer.handleWebSocket();
    
    leftEncoder.update();
    rightEncoder.update();
    
    // if (imu.isCalibrated()) {
    //     imu.update();
        
    //     if (millis() - lastIMULog >= 100) {
    //         lastIMULog = millis();
    //         TELEM_LOGF("IMU | AX:%.1f AY:%.1f AZ:%.1f GZ:%.2f° Heading:%.1f°", 
    //             imu.getAccelX(), imu.getAccelY(), imu.getAccelZ(), 
    //             imu.getGyroZ(), imu.getHeadingDegrees());
    //     }
    // }
    
    webServer.update();
    
    delay(10);
}