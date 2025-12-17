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
    void update();
    
    void setVelocity(float leftVel, float rightVel);

    void setFeedforwardGain(float gain);
    void setDeadzone(float deadzone);
    float getFeedforwardGain() const { return feedforwardGain; }
    float getDeadzone() const { return deadzonePWM; }
    
    void setPWMToVelocityPolynomial(const float* coeffs, int degree);
    void setVelocityToPWMPolynomial(const float* coeffs, int degree);
    Polynomial& getPWMToVelocityPolynomial() { return pwmToVelocityPoly; }
    Polynomial& getVelocityToPWMPolynomial() { return velocityToPWMPoly; }
    
    void enablePolynomialMapping(bool enable) { usePolynomialMapping = enable; }
    bool isPolynomialMappingEnabled() const { return usePolynomialMapping; }
    
    void setPIDGains(float kp, float ki, float kd);
    void enablePID(bool enable);
    bool isPIDEnabled() const { return pidEnabled; }
    void getPIDGains(float& kp, float& ki, float& kd) const;
    
    void attachEncoders(Encoder* left, Encoder* right);
    
    float getLeftPWM() const { return leftPWM; }
    float getRightPWM() const { return rightPWM; }
    
    float getLeftVelocityError() const { return leftVelError; }
    float getRightVelocityError() const { return rightVelError; }
    
private:
    Encoder* leftEncoder;
    Encoder* rightEncoder;

    float targetLeftVel;
    float targetRightVel;
    
    float feedforwardGain;
    float deadzonePWM;
    
    Polynomial pwmToVelocityPoly;
    Polynomial velocityToPWMPoly;
    bool usePolynomialMapping;
    
    PIDController leftPID;
    PIDController rightPID;
    bool pidEnabled;
    
    float leftPWM;
    float rightPWM;
    
    float leftVelError;
    float rightVelError;
    
    float velocityToPWM(float velocity);
};

#endif
