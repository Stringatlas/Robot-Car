#ifndef VELOCITYCONTROLLER_H
#define VELOCITYCONTROLLER_H

#include <Arduino.h>
#include "../hardware/Encoder.h"
#include "../utils/PIDController.h"
#include "../utils/Polynomial.h"

class VelocityController {
public:
    VelocityController();
    
    void begin();
    void update();  // Call this every loop
    
    // Set desired velocities (cm/s)
    void setVelocity(float leftVel, float rightVel);
    
    // Feedforward tuning parameters (legacy - kept for compatibility)
    void setFeedforwardGain(float gain);
    void setDeadzone(float deadzone);
    float getFeedforwardGain() const { return feedforwardGain; }
    float getDeadzone() const { return deadzonePWM; }
    
    // Polynomial mapping functions
    void setPWMToVelocityPolynomial(const float* coeffs, int degree);
    void setVelocityToPWMPolynomial(const float* coeffs, int degree);
    Polynomial& getPWMToVelocityPolynomial() { return pwmToVelocityPoly; }
    Polynomial& getVelocityToPWMPolynomial() { return velocityToPWMPoly; }
    
    // Enable/disable polynomial mapping (if false, uses legacy linear feedforward)
    void enablePolynomialMapping(bool enable) { usePolynomialMapping = enable; }
    bool isPolynomialMappingEnabled() const { return usePolynomialMapping; }
    
    // PID tuning parameters
    void setPIDGains(float kp, float ki, float kd);
    void enablePID(bool enable);
    bool isPIDEnabled() const { return pidEnabled; }
    void getPIDGains(float& kp, float& ki, float& kd) const;
    
    // Attach encoders
    void attachEncoders(Encoder* left, Encoder* right);
    
    // Get computed PWM values (for debugging)
    float getLeftPWM() const { return leftPWM; }
    float getRightPWM() const { return rightPWM; }
    
    // Get velocity errors (for tuning feedback)
    float getLeftVelocityError() const { return leftVelError; }
    float getRightVelocityError() const { return rightVelError; }
    
private:
    Encoder* leftEncoder;
    Encoder* rightEncoder;
    
    // Desired velocities (cm/s)
    float targetLeftVel;
    float targetRightVel;
    
    // Feedforward parameters (legacy)
    float feedforwardGain;  // PWM per cm/s
    float deadzonePWM;      // Minimum PWM to overcome static friction
    
    // Polynomial mappings
    Polynomial pwmToVelocityPoly;    // Forward model: PWM -> velocity
    Polynomial velocityToPWMPoly;    // Inverse model: velocity -> PWM
    bool usePolynomialMapping;
    
    // PID controllers
    PIDController leftPID;
    PIDController rightPID;
    bool pidEnabled;
    
    // Computed PWM outputs
    float leftPWM;
    float rightPWM;
    
    // Velocity tracking errors
    float leftVelError;
    float rightVelError;
    
    // Convert velocity to PWM using feedforward
    float velocityToPWM(float velocity);
};

#endif
