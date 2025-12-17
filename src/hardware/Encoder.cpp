#include "Encoder.h"

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

    if (deltaTime >= UPDATE_INTERVAL_MS)
    {
        long deltaCount = count - lastCount;
        float revolutions = (float)deltaCount / ppr;
        float distance = revolutions * PI * wheelDiameter;
        float timeSec = deltaTime / 1000.0;
        
        velocity = distance / timeSec;
        
        lastCount = count;
        lastTimeMs = currentTime;
    }
}

float Encoder::getDistance() const {
    float revolutions = (float)count / ppr;
    return revolutions * PI * wheelDiameter;  // cm
}

float Encoder::getRPM() const {
    float circumference = PI * wheelDiameter;
    float rps = velocity / circumference;
    return rps * 60.0;
}

void Encoder::handleInterruptA() {
    int A = digitalRead(pinA);
    int B = digitalRead(pinB);
    
    if (A != lastA) {
        int direction = 0;
        if (A == HIGH) {
            if (B == LOW) {
                direction = 1;
            } else {
                direction = -1;
            }
        } else {
            if (B == HIGH) {
                direction = 1;
            } else {
                direction = -1;
            }
        }
    
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
                direction = 1;
            } else {
                direction = -1;
            }
        } else {
            if (A == LOW) {
                direction = 1;
            } else {
                direction = -1;
            }
        }
        
        count += reversed ? -direction : direction;
        lastB = B;
    }
}

void IRAM_ATTR Encoder::isrA_Left() {
    if (leftInstance) leftInstance->handleInterruptA();
}

void IRAM_ATTR Encoder::isrB_Left() {
    if (leftInstance) leftInstance->handleInterruptB();
}

void IRAM_ATTR Encoder::isrA_Right() {
    if (rightInstance) rightInstance->handleInterruptA();
}

void IRAM_ATTR Encoder::isrB_Right() {
    if (rightInstance) rightInstance->handleInterruptB();
}
