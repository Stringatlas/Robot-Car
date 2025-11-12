#include "Encoder.h"

// Initialize static members
Encoder* Encoder::leftInstance = nullptr;
Encoder* Encoder::rightInstance = nullptr;

Encoder::Encoder(int pinA, int pinB, int ppr, float wheelDiameter, bool reversed)
    : pinA(pinA), pinB(pinB), ppr(ppr), wheelDiameter(wheelDiameter), reversed(reversed),
      count(0), lastA(0), lastB(0), lastTimeMs(0), lastCount(0), velocity(0.0) {}

void Encoder::begin() {
    pinMode(pinA, INPUT_PULLUP);
    pinMode(pinB, INPUT_PULLUP);
    
    lastA = digitalRead(pinA);
    lastB = digitalRead(pinB);
    
    // Attach interrupts based on which instance this is
    if (this == leftInstance) {
        attachInterrupt(digitalPinToInterrupt(pinA), isrA_Left, CHANGE);
        attachInterrupt(digitalPinToInterrupt(pinB), isrB_Left, CHANGE);
    } else if (this == rightInstance) {
        attachInterrupt(digitalPinToInterrupt(pinA), isrA_Right, CHANGE);
        attachInterrupt(digitalPinToInterrupt(pinB), isrB_Right, CHANGE);
    }
    
    lastTimeMs = millis();
}

void Encoder::update() {
    unsigned long currentTime = millis();
    unsigned long deltaTime = currentTime - lastTimeMs;
    
    if (deltaTime >= 100) {  // Update every 100ms
        long deltaCount = count - lastCount;
        float revolutions = (float)deltaCount / ppr;
        float distance = revolutions * PI * wheelDiameter;  // cm
        float timeSec = deltaTime / 1000.0;
        
        velocity = distance / timeSec;  // cm/s
        
        lastCount = count;
        lastTimeMs = currentTime;
    }
}

float Encoder::getDistance() const {
    float revolutions = (float)count / ppr;
    return revolutions * PI * wheelDiameter;  // cm
}

float Encoder::getRPM() const {
    // velocity is in cm/s, convert to RPM
    float circumference = PI * wheelDiameter;  // cm
    float rps = velocity / circumference;
    return rps * 60.0;  // RPM
}

void Encoder::handleInterruptA() {
    int A = digitalRead(pinA);
    int B = digitalRead(pinB);
    
    if (A != lastA) {
        int direction = 0;
        if (A == HIGH) {
            if (B == LOW) {
                direction = 1;  // Clockwise
            } else {
                direction = -1;  // Counterclockwise
            }
        } else {
            if (B == HIGH) {
                direction = 1;  // Clockwise
            } else {
                direction = -1;  // Counterclockwise
            }
        }
        
        // Apply reversal if needed
        count += reversed ? -direction : direction;
        lastA = A;
    }
}

void Encoder::handleInterruptB() {
    int A = digitalRead(pinA);
    int B = digitalRead(pinB);
    
    if (B != lastB) {
        int direction = 0;
        if (B == HIGH) {
            if (A == HIGH) {
                direction = 1;  // Clockwise
            } else {
                direction = -1;  // Counterclockwise
            }
        } else {
            if (A == LOW) {
                direction = 1;  // Clockwise
            } else {
                direction = -1;  // Counterclockwise
            }
        }
        
        // Apply reversal if needed
        count += reversed ? -direction : direction;
        lastB = B;
    }
}

// Static ISR callbacks for left encoder
void IRAM_ATTR Encoder::isrA_Left() {
    if (leftInstance) leftInstance->handleInterruptA();
}

void IRAM_ATTR Encoder::isrB_Left() {
    if (leftInstance) leftInstance->handleInterruptB();
}

// Static ISR callbacks for right encoder
void IRAM_ATTR Encoder::isrA_Right() {
    if (rightInstance) rightInstance->handleInterruptA();
}

void IRAM_ATTR Encoder::isrB_Right() {
    if (rightInstance) rightInstance->handleInterruptB();
}
