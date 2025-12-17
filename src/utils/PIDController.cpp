#include "PIDController.h"

PIDController::PIDController(float kp, float ki, float kd)
    : kp(kp), ki(ki), kd(kd), integral(0), previousError(0), lastTime(0),
      outputMin(-255), outputMax(255), integralMin(-100), integralMax(100) {}

void PIDController::setGains(float kp, float ki, float kd) {
    this->kp = kp;
    this->ki = ki;
    this->kd = kd;
}

void PIDController::setOutputLimits(float min, float max) {
    outputMin = min;
    outputMax = max;
}

void PIDController::setIntegralLimits(float min, float max) {
    integralMin = min;
    integralMax = max;
}

float PIDController::compute(float setpoint, float measurement) {
    unsigned long now = millis();
    float dt = (now - lastTime) / 1000.0;
    
    if (lastTime == 0 || dt <= 0 || dt > 1.0) {
        lastTime = now;
        previousError = setpoint - measurement;
        return 0;
    }
    
    // Calculate error
    float error = setpoint - measurement;
    
    // Proportional term
    float pTerm = kp * error;
    
    // Integral term
    integral += error * dt;
    integral = constrain(integral, integralMin, integralMax);  // Anti-windup
    float iTerm = ki * integral;
    
    // Derivative term
    float derivative = (error - previousError) / dt;
    float dTerm = kd * derivative;
    
    // Calculate total output
    float output = pTerm + iTerm + dTerm;
    output = constrain(output, outputMin, outputMax);
    
    // Save state
    previousError = error;
    lastTime = now;
    
    return output;
}

void PIDController::reset() {
    integral = 0;
    previousError = 0;
    lastTime = 0;
}
