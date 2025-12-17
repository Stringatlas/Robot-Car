#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>
#include <vector>

enum class LogType { Info, Warning, Error, Debug, Update, Command, Success };

class AsyncWebSocket;

class Telemetry {
public:
    static Telemetry& getInstance();

    void begin(AsyncWebSocket* wsPtr);
    void log(const String& message);
    void log(const String& message, LogType type);
    void logf(const char* format, ...);
    void logf(LogType type, const char* format, ...);

    // Get recent logs for new connections
    std::vector<String> getRecentLogs(size_t count = 50);

private:
    Telemetry();
    AsyncWebSocket* ws;
    std::vector<String> logBuffer;
    static const size_t MAX_LOG_BUFFER = 100;

    void addToBuffer(const String& message);
    void broadcast(const String& message, const String& logType);
};


// Convenience macros for all log types
#define TELEM_LOG(msg) Telemetry::getInstance().log(msg)
#define TELEM_LOG_INFO(msg) Telemetry::getInstance().log(msg, LogType::Info)
#define TELEM_LOG_WARNING(msg) Telemetry::getInstance().log(msg, LogType::Warning)
#define TELEM_LOG_ERROR(msg) Telemetry::getInstance().log(msg, LogType::Error)
#define TELEM_LOG_DEBUG(msg) Telemetry::getInstance().log(msg, LogType::Debug)
#define TELEM_LOG_UPDATE(msg) Telemetry::getInstance().log(msg, LogType::Update)
#define TELEM_LOG_COMMAND(msg) Telemetry::getInstance().log(msg, LogType::Command)
#define TELEM_LOG_SUCCESS(msg) Telemetry::getInstance().log(msg, LogType::Success)

#define TELEM_LOGF(...) Telemetry::getInstance().logf(__VA_ARGS__)
#define TELEM_LOGF_INFO(fmt, ...) Telemetry::getInstance().logf(LogType::Info, fmt, ##__VA_ARGS__)
#define TELEM_LOGF_WARNING(fmt, ...) Telemetry::getInstance().logf(LogType::Warning, fmt, ##__VA_ARGS__)
#define TELEM_LOGF_ERROR(fmt, ...) Telemetry::getInstance().logf(LogType::Error, fmt, ##__VA_ARGS__)
#define TELEM_LOGF_DEBUG(fmt, ...) Telemetry::getInstance().logf(LogType::Debug, fmt, ##__VA_ARGS__)
#define TELEM_LOGF_UPDATE(fmt, ...) Telemetry::getInstance().logf(LogType::Update, fmt, ##__VA_ARGS__)
#define TELEM_LOGF_COMMAND(fmt, ...) Telemetry::getInstance().logf(LogType::Command, fmt, ##__VA_ARGS__)
#define TELEM_LOGF_SUCCESS(fmt, ...) Telemetry::getInstance().logf(LogType::Success, fmt, ##__VA_ARGS__)


#endif
