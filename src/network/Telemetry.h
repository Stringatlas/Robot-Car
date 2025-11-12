#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>
#include <vector>

class AsyncWebSocket;

class Telemetry {
public:
    static Telemetry& getInstance();
    
    void begin(AsyncWebSocket* wsPtr);
    void log(const String& message);
    void logf(const char* format, ...);
    
    // Get recent logs for new connections
    std::vector<String> getRecentLogs(size_t count = 50);
    
private:
    Telemetry();
    AsyncWebSocket* ws;
    std::vector<String> logBuffer;
    static const size_t MAX_LOG_BUFFER = 100;
    
    void addToBuffer(const String& message);
    void broadcast(const String& message);
};

// Convenience macros for easier migration from Serial.print
#define TELEM_LOG(msg) Telemetry::getInstance().log(msg)
#define TELEM_LOGF(...) Telemetry::getInstance().logf(__VA_ARGS__)

#endif
