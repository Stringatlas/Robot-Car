#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Arduino.h>

// Define HTTP methods BEFORE including ESPAsyncWebServer
#ifndef HTTP_GET
#define HTTP_GET     0b00000001
#define HTTP_POST    0b00000010
#define HTTP_DELETE  0b00000100
#define HTTP_PUT     0b00001000
#define HTTP_PATCH   0b00010000
#define HTTP_HEAD    0b00100000
#define HTTP_OPTIONS 0b01000000
#define HTTP_ANY     0b01111111
#endif

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <LittleFS.h>
#include "../hardware/Encoder.h"
#include "../drive/DriveController.h"
#include "../hardware/BatteryMonitor.h"
#include "../drive/VelocityController.h"
#include "../utils/ConfigManager.h"

// Control modes for robot operation
enum class ControlMode {
    IDLE,           // No control active
    JOYSTICK,       // Joystick driving mode
    DIRECT_MOTOR,   // Direct motor power control
    VELOCITY,       // Velocity setpoint control
    CALIBRATION     // Automatic calibration sweep
};

class WebServerManager {
private:
    AsyncWebServer server;
    AsyncWebSocket ws;
    Encoder* leftEncoder;
    Encoder* rightEncoder;
    DriveController* driveController;
    BatteryMonitor* batteryMonitor;
    VelocityController* velocityController;
    ConfigManager* configManager;
    unsigned long lastUpdate;
    uint32_t controllingClientId;  // ID of client that has control (0 = none)
    
    // Control mode state
    ControlMode currentMode;
    unsigned long lastCommandTime;  // Track when last command received
    static constexpr unsigned long CONTROL_TIMEOUT_MS = 500;  // Revert to IDLE after 500ms
    
    // Calibration state
    bool calibrationActive;
    String calibrationMotor;  // "left", "right", or "both"
    int calibrationPWM;
    int calibrationStartPWM;
    int calibrationEndPWM;
    int calibrationStepSize;
    unsigned long calibrationHoldTime;
    unsigned long calibrationStepStart;

public:
    WebServerManager(int port);
    
    void begin(Encoder* left, Encoder* right, DriveController* drive, BatteryMonitor* battery, VelocityController* velCtrl, ConfigManager* config);
    void setupRoutes();
    void handleWebSocket();
    void broadcastEncoderData();
    void broadcastControlStatus();
    void update();  // Call this in loop - handles control mode state machine and calibration
    void updateCalibration();  // Internal - run calibration state machine
    bool isCalibrationActive() const { return calibrationActive; }  // Check if calibration is running
    ControlMode getControlMode() const { return currentMode; }
    
private:
    void setControlMode(ControlMode mode);
    void checkControlTimeout();
    
private:
    void handleRoot(AsyncWebServerRequest *request);
    void handleEncoderAPI(AsyncWebServerRequest *request);
    void handleUpload(AsyncWebServerRequest *request, String filename, 
                     size_t index, uint8_t *data, size_t len, bool final);
    void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                         AwsEventType type, void *arg, uint8_t *data, size_t len);
};

#endif // WEBSERVER_H
