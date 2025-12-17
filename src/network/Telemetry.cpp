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

void Telemetry::log(const String& message, LogType type) {
    String typeStr;
    switch (type) {
        case LogType::Info: typeStr = "info"; break;
        case LogType::Warning: typeStr = "warning"; break;
        case LogType::Error: typeStr = "error"; break;
        case LogType::Debug: typeStr = "debug"; break;
        case LogType::Update: typeStr = "update"; break;
        case LogType::Command: typeStr = "command"; break;
        case LogType::Success: typeStr = "success"; break;
        default: typeStr = "info"; break;
    }
    Serial.println(message);
    String timestampedMsg = "[" + String(millis()) + "ms] " + message;
    addToBuffer(timestampedMsg);
    broadcast(timestampedMsg, typeStr);
}

void Telemetry::log(const String& message) {
    log(message, LogType::Info);
}

void Telemetry::logf(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    log(String(buffer), LogType::Info);
}

void Telemetry::logf(LogType type, const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    log(String(buffer), type);
}

void Telemetry::addToBuffer(const String& message) {
    logBuffer.push_back(message);
    
    if (logBuffer.size() > MAX_LOG_BUFFER) {
        logBuffer.erase(logBuffer.begin());
    }
}

void Telemetry::broadcast(const String& message, const String& logType) {
    if (ws && ws->count() > 0) {
        // Escape quotes and newlines for JSON
        String escaped = message;
        escaped.replace("\"", "\\\"");
        escaped.replace("\n", "\\n");

        String json = "{\"type\":\"log\",\"logType\":\"" + logType + "\",\"message\":\"" + escaped + "\"}";
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
