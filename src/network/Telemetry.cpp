#include "Telemetry.h"
#include <ESPAsyncWebServer.h>

Telemetry::Telemetry() : ws(nullptr) {}

Telemetry& Telemetry::getInstance() {
    static Telemetry instance;
    return instance;
}

void Telemetry::begin(AsyncWebSocket* wsPtr) {
    ws = wsPtr;
}

void Telemetry::log(const String& message) {
    Serial.println(message);
    
    // Add timestamp
    String timestampedMsg = "[" + String(millis()) + "ms] " + message;
    
    addToBuffer(timestampedMsg);
    broadcast(timestampedMsg);
}

void Telemetry::logf(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    log(String(buffer));
}

void Telemetry::addToBuffer(const String& message) {
    logBuffer.push_back(message);
    
    // Keep buffer size limited
    if (logBuffer.size() > MAX_LOG_BUFFER) {
        logBuffer.erase(logBuffer.begin());
    }
}

void Telemetry::broadcast(const String& message) {
    if (ws && ws->count() > 0) {
        // Escape quotes and newlines for JSON
        String escaped = message;
        escaped.replace("\"", "\\\"");
        escaped.replace("\n", "\\n");
        
        String json = "{\"type\":\"log\",\"message\":\"" + escaped + "\"}";
        ws->textAll(json);
    }
}

std::vector<String> Telemetry::getRecentLogs(size_t count) {
    if (logBuffer.size() <= count) {
        return logBuffer;
    }
    
    return std::vector<String>(
        logBuffer.end() - count,
        logBuffer.end()
    );
}
