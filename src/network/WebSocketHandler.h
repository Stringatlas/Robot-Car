#ifndef WEBSOCKETHANDLER_H
#define WEBSOCKETHANDLER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <functional>

class WebSocketHandler {
public:
    using MessageCallback = std::function<void(uint32_t clientId, const String& message)>;
    using BinaryMessageCallback = std::function<void(uint32_t clientId, const uint8_t* data, size_t len)>;
    using ConnectionCallback = std::function<void(uint32_t clientId, bool connected)>;

    WebSocketHandler(const char* path);
    
    void begin(AsyncWebServer* server);
    void onMessage(MessageCallback callback);
    void onBinaryMessage(BinaryMessageCallback callback);
    void onConnection(ConnectionCallback callback);
    
    void sendText(uint32_t clientId, const String& message);
    void broadcastText(const String& message);
    void sendJson(uint32_t clientId, const JsonDocument& doc);
    void broadcastJson(const JsonDocument& doc);
    
    void sendBinary(uint32_t clientId, const uint8_t* data, size_t len);
    void broadcastBinary(const uint8_t* data, size_t len);
    
    bool parseJson(const String& message, JsonDocument& doc);
    
    uint32_t getClientCount() const;
    void cleanup();
    
    AsyncWebSocket* getWebSocket() { return &ws; }

private:
    AsyncWebSocket ws;
    MessageCallback messageCallback;
    BinaryMessageCallback binaryMessageCallback;
    ConnectionCallback connectionCallback;
    
    void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                         AwsEventType type, void* arg, uint8_t* data, size_t len);
};

#endif
