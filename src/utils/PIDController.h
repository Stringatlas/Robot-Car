#ifndef PIDCONTROLLER_H
#define PIDCONTROLLER_H

#include <Arduino.h>

class PIDController {
private:
    float kp;
    float ki;
    float kd;
    
    float integral;
    float previousError;
    unsigned long lastTime;
    
    float outputMin;
    float outputMax;
    float integralMin;
    float integralMax;

public:
    PIDController(float kp = 0.0, float ki = 0.0, float kd = 0.0);
    
    void setGains(float kp, float ki, float kd);
    void setOutputLimits(float min, float max);
    void setIntegralLimits(float min, float max);
    
    float compute(float setpoint, float measurement);
    void reset();
    
    // Getters
    float getKp() const { return kp; }
    float getKi() const { return ki; }
    float getKd() const { return kd; }
    float getIntegral() const { return integral; }
};

#endif // PIDCONTROLLER_H
