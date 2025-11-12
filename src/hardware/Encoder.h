#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

class Encoder {
private:
    int pinA;
    int pinB;
    int ppr;  // Pulses per revolution
    float wheelDiameter;  // in cm
    bool reversed;  // Reverse counting direction
    
    volatile long count;
    volatile int lastA;
    volatile int lastB;
    
    unsigned long lastTimeMs;
    long lastCount;
    float velocity;  // cm/s
    
    // Static callback handlers (required for attachInterrupt)
    static void IRAM_ATTR isrA_Left();
    static void IRAM_ATTR isrB_Left();
    static void IRAM_ATTR isrA_Right();
    static void IRAM_ATTR isrB_Right();
    
    // Static instances for ISR access
    static Encoder* leftInstance;
    static Encoder* rightInstance;
    
    void handleInterruptA();
    void handleInterruptB();

public:
    Encoder(int pinA, int pinB, int ppr, float wheelDiameter, bool reversed = false);
    
    void begin();
    void update();  // Call this regularly to update velocity
    
    // Getters
    long getCount() const { return count; }
    float getRevolutions() const { return (float)count / ppr; }
    float getDegrees() const { return (count % ppr) * (360.0 / ppr); }
    float getRadians() const { return (count % ppr) * (2.0 * PI / ppr); }
    float getDistance() const;  // Returns distance traveled in cm
    float getVelocity() const { return velocity; }  // Returns velocity in cm/s
    float getRPM() const;  // Returns RPM
    
    // Setters
    void reset() { count = 0; lastCount = 0; }
    
    // Static registration methods
    static void registerLeft(Encoder* enc) { leftInstance = enc; }
    static void registerRight(Encoder* enc) { rightInstance = enc; }
};

#endif // ENCODER_H
