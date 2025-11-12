#ifndef JSONBUILDER_H
#define JSONBUILDER_H

#include <Arduino.h>

/**
 * JsonBuilder - Efficient JSON string builder for WebSocket responses
 * Uses a pre-allocated buffer and avoids repeated string allocations
 */
class JsonBuilder {
public:
    JsonBuilder(size_t capacity = 512) : buffer(""), capacity(capacity) {
        buffer.reserve(capacity);
    }
    
    // Start a new JSON object
    JsonBuilder& startObject() {
        buffer += "{";
        needsComma = false;
        return *this;
    }
    
    // End JSON object
    JsonBuilder& endObject() {
        buffer += "}";
        needsComma = true;
        return *this;
    }
    
    // Start a new JSON array
    JsonBuilder& startArray(const char* key = nullptr) {
        addComma();
        if (key) {
            buffer += "\"";
            buffer += key;
            buffer += "\":[";
        } else {
            buffer += "[";
        }
        needsComma = false;
        return *this;
    }
    
    // End JSON array
    JsonBuilder& endArray() {
        buffer += "]";
        needsComma = true;
        return *this;
    }
    
    // Add string field
    JsonBuilder& addString(const char* key, const String& value) {
        addComma();
        buffer += "\"";
        buffer += key;
        buffer += "\":\"";
        buffer += value;
        buffer += "\"";
        needsComma = true;
        return *this;
    }
    
    // Add string field (const char*)
    JsonBuilder& addString(const char* key, const char* value) {
        addComma();
        buffer += "\"";
        buffer += key;
        buffer += "\":\"";
        buffer += value;
        buffer += "\"";
        needsComma = true;
        return *this;
    }
    
    // Add integer field
    JsonBuilder& addInt(const char* key, int value) {
        addComma();
        buffer += "\"";
        buffer += key;
        buffer += "\":";
        buffer += String(value);
        needsComma = true;
        return *this;
    }
    
    // Add long integer field
    JsonBuilder& addLong(const char* key, long value) {
        addComma();
        buffer += "\"";
        buffer += key;
        buffer += "\":";
        buffer += String(value);
        needsComma = true;
        return *this;
    }
    
    // Add float field with precision
    JsonBuilder& addFloat(const char* key, float value, int decimals = 2) {
        addComma();
        buffer += "\"";
        buffer += key;
        buffer += "\":";
        buffer += String(value, decimals);
        needsComma = true;
        return *this;
    }
    
    // Add double field with precision
    JsonBuilder& addDouble(const char* key, double value, int decimals = 2) {
        addComma();
        buffer += "\"";
        buffer += key;
        buffer += "\":";
        buffer += String(value, decimals);
        needsComma = true;
        return *this;
    }
    
    // Add boolean field
    JsonBuilder& addBool(const char* key, bool value) {
        addComma();
        buffer += "\"";
        buffer += key;
        buffer += "\":";
        buffer += value ? "true" : "false";
        needsComma = true;
        return *this;
    }
    
    // Add null field
    JsonBuilder& addNull(const char* key) {
        addComma();
        buffer += "\"";
        buffer += key;
        buffer += "\":null";
        needsComma = true;
        return *this;
    }
    
    // Start nested object
    JsonBuilder& startNestedObject(const char* key) {
        addComma();
        buffer += "\"";
        buffer += key;
        buffer += "\":{";
        needsComma = false;
        return *this;
    }
    
    // Add raw JSON value (no quotes, no escaping)
    JsonBuilder& addRaw(const char* key, const String& rawJson) {
        addComma();
        buffer += "\"";
        buffer += key;
        buffer += "\":";
        buffer += rawJson;
        needsComma = true;
        return *this;
    }
    
    // Clear the builder for reuse
    void clear() {
        buffer = "";
        needsComma = false;
    }
    
    // Get the final JSON string
    String toString() const {
        return buffer;
    }
    
    // Get C string
    const char* c_str() const {
        return buffer.c_str();
    }
    
    // Get current length
    size_t length() const {
        return buffer.length();
    }
    
private:
    String buffer;
    size_t capacity;
    bool needsComma = false;
    
    void addComma() {
        if (needsComma) {
            buffer += ",";
        }
    }
};

/**
 * Helper class for building encoder telemetry JSON
 */
class EncoderJsonBuilder {
public:
    static String buildEncoderData(
        long leftCount, float leftRevs, float leftDist, float leftVel, float leftRPM,
        long rightCount, float rightRevs, float rightDist, float rightVel, float rightRPM,
        float battery,
        float motorLeftPWM, float motorRightPWM,
        float leftVelError, float rightVelError
    ) {
        JsonBuilder json(512);
        
        json.startObject()
            .startNestedObject("left")
                .addLong("count", leftCount)
                .addFloat("revolutions", leftRevs, 2)
                .addFloat("distance", leftDist, 2)
                .addFloat("velocity", leftVel, 2)
                .addFloat("rpm", leftRPM, 1)
            .endObject()
            .startNestedObject("right")
                .addLong("count", rightCount)
                .addFloat("revolutions", rightRevs, 2)
                .addFloat("distance", rightDist, 2)
                .addFloat("velocity", rightVel, 2)
                .addFloat("rpm", rightRPM, 1)
            .endObject()
            .addFloat("battery", battery, 2)
            .addFloat("motorLeft", motorLeftPWM, 0)
            .addFloat("motorRight", motorRightPWM, 0)
            .addFloat("leftVelError", leftVelError, 2)
            .addFloat("rightVelError", rightVelError, 2)
        .endObject();
        
        return json.toString();
    }
    
    static String buildSimpleEncoderData(
        long leftCount, float leftRevs, float leftDist, float leftVel, float leftRPM,
        long rightCount, float rightRevs, float rightDist, float rightVel, float rightRPM,
        float battery
    ) {
        JsonBuilder json(384);
        
        json.startObject()
            .startNestedObject("left")
                .addLong("count", leftCount)
                .addFloat("revolutions", leftRevs, 2)
                .addFloat("distance", leftDist, 2)
                .addFloat("velocity", leftVel, 2)
                .addFloat("rpm", leftRPM, 1)
            .endObject()
            .startNestedObject("right")
                .addLong("count", rightCount)
                .addFloat("revolutions", rightRevs, 2)
                .addFloat("distance", rightDist, 2)
                .addFloat("velocity", rightVel, 2)
                .addFloat("rpm", rightRPM, 1)
            .endObject()
            .addFloat("battery", battery, 2)
        .endObject();
        
        return json.toString();
    }
};

/**
 * Helper class for building configuration JSON
 */
class ConfigJsonBuilder {
public:
    static String buildConfigResponse(
        float ffGain, float deadzone, bool pidEnabled,
        float pidKp, float pidKi, float pidKd,
        bool polyEnabled
    ) {
        JsonBuilder json(256);
        
        json.startObject()
            .addFloat("feedforwardGain", ffGain, 3)
            .addFloat("deadzonePWM", deadzone, 1)
            .addBool("pidEnabled", pidEnabled)
            .addFloat("pidKp", pidKp, 3)
            .addFloat("pidKi", pidKi, 3)
            .addFloat("pidKd", pidKd, 3)
            .addBool("polynomialEnabled", polyEnabled)
        .endObject();
        
        return json.toString();
    }
};

/**
 * Helper class for building WebSocket control messages
 */
class WebSocketMessageBuilder {
public:
    static String buildWelcomeMessage(uint32_t clientId) {
        JsonBuilder json(64);
        json.startObject()
            .addString("type", "welcome")
            .addInt("clientId", clientId)
        .endObject();
        return json.toString();
    }
    
    static String buildControlStatus(uint32_t controllingClientId) {
        JsonBuilder json(64);
        json.startObject()
            .addString("type", "control")
            .addInt("controllingClientId", controllingClientId)
        .endObject();
        return json.toString();
    }
    
    static String buildLogMessage(const String& message) {
        JsonBuilder json(256);
        String escaped = message;
        escaped.replace("\"", "\\\"");
        escaped.replace("\n", "\\n");
        json.startObject()
            .addString("type", "log")
            .addString("message", escaped)
        .endObject();
        return json.toString();
    }
    
    static String buildVelocityError(float leftError, float rightError, bool pidEnabled) {
        String msg = "VEL_ERROR:";
        msg += String(leftError, 2);
        msg += ",";
        msg += String(rightError, 2);
        msg += ",";
        msg += pidEnabled ? "true" : "false";
        return msg;
    }
    
    static String buildCalibrationPoint(int pwm, float leftVel, float rightVel) {
        String msg = "CALIBRATION_POINT:";
        msg += String(pwm);
        msg += ",";
        msg += String(leftVel, 2);
        msg += ",";
        msg += String(rightVel, 2);
        return msg;
    }
    
    static String buildCalibrationProgress(int currentPWM, int endPWM, int startPWM) {
        int progress = map(currentPWM, startPWM, endPWM, 0, 100);
        String msg = "CALIBRATION_PROGRESS:PWM ";
        msg += String(currentPWM);
        msg += "/";
        msg += String(endPWM);
        msg += " (";
        msg += String(progress);
        msg += "%)";
        return msg;
    }
    
    static String buildCommandAck(const char* command, const String& value) {
        String msg = "COMMAND_ACK:";
        msg += command;
        msg += ":";
        msg += value;
        return msg;
    }
};

#endif // JSONBUILDER_H
