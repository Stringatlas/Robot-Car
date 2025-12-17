#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

/**
 * Persistent storage for robot configuration and tuning parameters
 * Stores configuration in JSON format on LittleFS filesystem
 */
class ConfigManager {
public:
    struct Config {
        // Velocity Controller Parameters
        float feedforwardGain;
        float deadzonePWM;
        bool pidEnabled;
        float pidKp;
        float pidKi;
        float pidKd;
        
        // Polynomial Coefficients (Velocity -> PWM)
        bool polynomialEnabled;
        float vel2pwm_a0;
        float vel2pwm_a1;
        float vel2pwm_a2;
        float vel2pwm_a3;
        
        // Polynomial Coefficients (PWM -> Velocity)
        float pwm2vel_b0;
        float pwm2vel_b1;
        float pwm2vel_b2;
        float pwm2vel_b3;
        
        // Default constructor with sensible defaults
        Config() :
            feedforwardGain(3.0f),
            deadzonePWM(60.0f),
            pidEnabled(false),
            pidKp(0.0f),
            pidKi(0.0f),
            pidKd(0.0f),
            polynomialEnabled(false),
            vel2pwm_a0(0.0f),
            vel2pwm_a1(1.0f),
            vel2pwm_a2(0.0f),
            vel2pwm_a3(0.0f),
            pwm2vel_b0(0.0f),
            pwm2vel_b1(1.0f),
            pwm2vel_b2(0.0f),
            pwm2vel_b3(0.0f) {}
    };
    
    ConfigManager(const char* configPath = "/config.json");
    
    bool load();
    
    bool save();
    
    void reset();
    
    Config& getConfig() { return config; }

    const Config& getConfig() const { return config; }

    bool updateFromJson(const String& jsonStr);
    
    String toJson() const;
    
    void print() const;

private:
    const char* configPath;
    Config config;
};

#endif
