#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

class Encoder {
private:
    static constexpr unsigned long UPDATE_INTERVAL_MS = 100;
    int pinA;
    int pinB;
    int ppr;
    float wheelDiameter;
    bool reversed;
    
    volatile long count;
    volatile int lastA;
    volatile int lastB;
    
    unsigned long lastTimeMs;
    long lastCount;
    float velocity;
    
    static void IRAM_ATTR isrA_Left();
    static void IRAM_ATTR isrB_Left();
    static void IRAM_ATTR isrA_Right();
    static void IRAM_ATTR isrB_Right();
    
    static Encoder* leftInstance;
    static Encoder* rightInstance;
    
    void handleInterruptA();
    void handleInterruptB();

public:
    Encoder(int pinA, int pinB, int ppr, float wheelDiameter, bool reversed = false);
    
    void begin();
    void update();
    
    long getCount() const { return count; }
    float getRevolutions() const { return (float)count / ppr; }
    float getDegrees() const { return (count % ppr) * (360.0 / ppr); }
    float getRadians() const { return (count % ppr) * (2.0 * PI / ppr); }
    float getDistance() const;
    float getVelocity() const { return velocity; }
    float getRPM() const;
    
    void reset() { count = 0; lastCount = 0; }
    
    static void registerLeft(Encoder* enc) { leftInstance = enc; }
    static void registerRight(Encoder* enc) { rightInstance = enc; }
};

#endif
