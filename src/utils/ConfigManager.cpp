#include "ConfigManager.h"

ConfigManager::ConfigManager(const char* path) : configPath(path) {
    config = Config();  // Initialize with defaults
}

bool ConfigManager::load() {
    if (!LittleFS.exists(configPath)) {
        Serial.println("Config file not found, using defaults");
        return false;
    }
    
    File file = LittleFS.open(configPath, "r");
    if (!file) {
        Serial.println("Failed to open config file");
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.print("Failed to parse config: ");
        Serial.println(error.c_str());
        return false;
    }
    
    // Load velocity controller parameters
    config.feedforwardGain = doc["feedforwardGain"] | 3.0f;
    config.deadzonePWM = doc["deadzonePWM"] | 60.0f;
    config.pidEnabled = doc["pidEnabled"] | false;
    config.pidKp = doc["pidKp"] | 0.0f;
    config.pidKi = doc["pidKi"] | 0.0f;
    config.pidKd = doc["pidKd"] | 0.0f;
    
    // Load polynomial parameters
    config.polynomialEnabled = doc["polynomialEnabled"] | false;
    config.vel2pwm_a0 = doc["vel2pwm_a0"] | 0.0f;
    config.vel2pwm_a1 = doc["vel2pwm_a1"] | 1.0f;
    config.vel2pwm_a2 = doc["vel2pwm_a2"] | 0.0f;
    config.vel2pwm_a3 = doc["vel2pwm_a3"] | 0.0f;
    
    config.pwm2vel_b0 = doc["pwm2vel_b0"] | 0.0f;
    config.pwm2vel_b1 = doc["pwm2vel_b1"] | 1.0f;
    config.pwm2vel_b2 = doc["pwm2vel_b2"] | 0.0f;
    config.pwm2vel_b3 = doc["pwm2vel_b3"] | 0.0f;
    
    Serial.println("✓ Configuration loaded successfully");
    return true;
}

bool ConfigManager::save() {
    JsonDocument doc;
    
    // Save velocity controller parameters
    doc["feedforwardGain"] = config.feedforwardGain;
    doc["deadzonePWM"] = config.deadzonePWM;
    doc["pidEnabled"] = config.pidEnabled;
    doc["pidKp"] = config.pidKp;
    doc["pidKi"] = config.pidKi;
    doc["pidKd"] = config.pidKd;
    
    // Save polynomial parameters
    doc["polynomialEnabled"] = config.polynomialEnabled;
    doc["vel2pwm_a0"] = config.vel2pwm_a0;
    doc["vel2pwm_a1"] = config.vel2pwm_a1;
    doc["vel2pwm_a2"] = config.vel2pwm_a2;
    doc["vel2pwm_a3"] = config.vel2pwm_a3;
    
    doc["pwm2vel_b0"] = config.pwm2vel_b0;
    doc["pwm2vel_b1"] = config.pwm2vel_b1;
    doc["pwm2vel_b2"] = config.pwm2vel_b2;
    doc["pwm2vel_b3"] = config.pwm2vel_b3;
    
    File file = LittleFS.open(configPath, "w");
    if (!file) {
        Serial.println("Failed to open config file for writing");
        return false;
    }
    
    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write config");
        file.close();
        return false;
    }
    
    file.close();
    Serial.println("✓ Configuration saved successfully");
    return true;
}

void ConfigManager::reset() {
    config = Config();  // Reset to defaults
    Serial.println("Configuration reset to defaults");
}

bool ConfigManager::updateFromJson(const String& jsonStr) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonStr);
    
    if (error) {
        Serial.print("Failed to parse JSON: ");
        Serial.println(error.c_str());
        return false;
    }
    
    // Update only provided fields
    if (doc["feedforwardGain"].is<float>()) config.feedforwardGain = doc["feedforwardGain"];
    if (doc["deadzonePWM"].is<float>()) config.deadzonePWM = doc["deadzonePWM"];
    if (doc["pidEnabled"].is<bool>()) config.pidEnabled = doc["pidEnabled"];
    if (doc["pidKp"].is<float>()) config.pidKp = doc["pidKp"];
    if (doc["pidKi"].is<float>()) config.pidKi = doc["pidKi"];
    if (doc["pidKd"].is<float>()) config.pidKd = doc["pidKd"];
    
    if (doc["polynomialEnabled"].is<bool>()) config.polynomialEnabled = doc["polynomialEnabled"];
    if (doc["vel2pwm_a0"].is<float>()) config.vel2pwm_a0 = doc["vel2pwm_a0"];
    if (doc["vel2pwm_a1"].is<float>()) config.vel2pwm_a1 = doc["vel2pwm_a1"];
    if (doc["vel2pwm_a2"].is<float>()) config.vel2pwm_a2 = doc["vel2pwm_a2"];
    if (doc["vel2pwm_a3"].is<float>()) config.vel2pwm_a3 = doc["vel2pwm_a3"];
    
    if (doc["pwm2vel_b0"].is<float>()) config.pwm2vel_b0 = doc["pwm2vel_b0"];
    if (doc["pwm2vel_b1"].is<float>()) config.pwm2vel_b1 = doc["pwm2vel_b1"];
    if (doc["pwm2vel_b2"].is<float>()) config.pwm2vel_b2 = doc["pwm2vel_b2"];
    if (doc["pwm2vel_b3"].is<float>()) config.pwm2vel_b3 = doc["pwm2vel_b3"];
    
    return true;
}

String ConfigManager::toJson() const {
    JsonDocument doc;
    
    doc["feedforwardGain"] = config.feedforwardGain;
    doc["deadzonePWM"] = config.deadzonePWM;
    doc["pidEnabled"] = config.pidEnabled;
    doc["pidKp"] = config.pidKp;
    doc["pidKi"] = config.pidKi;
    doc["pidKd"] = config.pidKd;
    
    doc["polynomialEnabled"] = config.polynomialEnabled;
    doc["vel2pwm_a0"] = config.vel2pwm_a0;
    doc["vel2pwm_a1"] = config.vel2pwm_a1;
    doc["vel2pwm_a2"] = config.vel2pwm_a2;
    doc["vel2pwm_a3"] = config.vel2pwm_a3;
    
    doc["pwm2vel_b0"] = config.pwm2vel_b0;
    doc["pwm2vel_b1"] = config.pwm2vel_b1;
    doc["pwm2vel_b2"] = config.pwm2vel_b2;
    doc["pwm2vel_b3"] = config.pwm2vel_b3;
    
    String output;
    serializeJson(doc, output);
    return output;
}

void ConfigManager::print() const {
    Serial.println("=== Current Configuration ===");
    Serial.printf("Feedforward Gain: %.3f PWM/(cm/s)\n", config.feedforwardGain);
    Serial.printf("Deadzone: %.1f PWM\n", config.deadzonePWM);
    Serial.printf("PID Enabled: %s\n", config.pidEnabled ? "Yes" : "No");
    Serial.printf("PID Gains: Kp=%.3f Ki=%.3f Kd=%.3f\n", config.pidKp, config.pidKi, config.pidKd);
    Serial.printf("Polynomial Mapping: %s\n", config.polynomialEnabled ? "Enabled" : "Disabled");
    if (config.polynomialEnabled) {
        Serial.printf("Vel->PWM: %.6f + %.6f*v + %.6f*v² + %.6f*v³\n", 
                     config.vel2pwm_a0, config.vel2pwm_a1, config.vel2pwm_a2, config.vel2pwm_a3);
        Serial.printf("PWM->Vel: %.6f + %.6f*p + %.6f*p² + %.6f*p³\n", 
                     config.pwm2vel_b0, config.pwm2vel_b1, config.pwm2vel_b2, config.pwm2vel_b3);
    }
    Serial.println("=============================");
}
