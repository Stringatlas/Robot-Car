#include "WebSocketHandler.h"
#include "Telemetry.h"

WebSocketHandler::WebSocketHandler(const char* path) 
    : ws(path), messageCallback(nullptr), binaryMessageCallback(nullptr), connectionCallback(nullptr) {}

void WebSocketHandler::begin(AsyncWebServer* server) {
    ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                     AwsEventType type, void* arg, uint8_t* data, size_t len) {
        this->onWebSocketEvent(server, client, type, arg, data, len);
    });
    server->addHandler(&ws);
    TELEM_LOG_INFO("WebSocket handler initialized");
}

void WebSocketHandler::onMessage(MessageCallback callback) {
    messageCallback = callback;
}

void WebSocketHandler::onBinaryMessage(BinaryMessageCallback callback) {
    binaryMessageCallback = callback;
}

void WebSocketHandler::onConnection(ConnectionCallback callback) {
    connectionCallback = callback;
}

void WebSocketHandler::onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                       AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        TELEM_LOGF_INFO("WebSocket client #%u connected from %s", 
                        client->id(), client->remoteIP().toString().c_str());
        if (connectionCallback) {
            connectionCallback(client->id(), true);
        }
    } 
    else if (type == WS_EVT_DISCONNECT) {
        TELEM_LOGF_INFO("WebSocket client #%u disconnected", client->id());
        if (connectionCallback) {
            connectionCallback(client->id(), false);
        }
    } 
    else if (type == WS_EVT_DATA) {
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len) {
            if (info->opcode == WS_TEXT) {
                data[len] = 0;
                String message = (char*)data;
                
                if (messageCallback) {
                    messageCallback(client->id(), message);
                }
            }
            else if (info->opcode == WS_BINARY) {
                if (binaryMessageCallback) {
                    binaryMessageCallback(client->id(), data, len);
                }
            }
        }
    }
}

void WebSocketHandler::sendText(uint32_t clientId, const String& message) {
    AsyncWebSocketClient* client = ws.client(clientId);
    if (client && client->status() == WS_CONNECTED) {
        client->text(message);
    }
}

void WebSocketHandler::broadcastText(const String& message) {
    ws.textAll(message);
}

void WebSocketHandler::sendJson(uint32_t clientId, const JsonDocument& doc) {
    String json;
    serializeJson(doc, json);
    sendText(clientId, json);
}

void WebSocketHandler::broadcastJson(const JsonDocument& doc) {
    String json;
    serializeJson(doc, json);
    broadcastText(json);
}

void WebSocketHandler::sendBinary(uint32_t clientId, const uint8_t* data, size_t len) {
    AsyncWebSocketClient* client = ws.client(clientId);
    if (client && client->status() == WS_CONNECTED) {
        client->binary(data, len);
    }
}

void WebSocketHandler::broadcastBinary(const uint8_t* data, size_t len) {
    ws.binaryAll(data, len);
}

bool WebSocketHandler::parseJson(const String& message, JsonDocument& doc) {
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        TELEM_LOGF_ERROR("JSON parse error: %s", error.c_str());
        return false;
    }
    return true;
}

uint32_t WebSocketHandler::getClientCount() const {
    return ws.count();
}

void WebSocketHandler::cleanup() {
    ws.cleanupClients();
}
