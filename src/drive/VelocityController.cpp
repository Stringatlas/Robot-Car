#include "VelocityController.h"
#include "./DriveController.h"
#include "../network/Telemetry.h"

extern DriveController driveController;

VelocityController::VelocityController() 
    : leftEncoder(nullptr), rightEncoder(nullptr),
      targetLeftVel(0), targetRightVel(0),
      feedforwardGain(3),
      deadzonePWM(60),
      pwmToVelocityPoly(),
      velocityToPWMPoly(),
      usePolynomialMapping(false),
      leftPID(0, 0, 0), rightPID(0, 0, 0), pidEnabled(false),
      leftPWM(0), rightPWM(0),
      leftVelError(0), rightVelError(0) {

    leftPID.setOutputLimits(-100, 100);
    rightPID.setOutputLimits(-100, 100);
}

void VelocityController::begin() {
    TELEM_LOG_SUCCESS("Velocity controller initialized");
    TELEM_LOGF_SUCCESS("  Feedforward gain: %.2f PWM/(cm/s)", feedforwardGain);
    TELEM_LOGF_SUCCESS("  Deadzone: %.0f PWM", deadzonePWM);
}

void VelocityController::attachEncoders(Encoder* left, Encoder* right) {
    leftEncoder = left;
    rightEncoder = right;
}

void VelocityController::setVelocity(float leftVel, float rightVel) {
    targetLeftVel = leftVel;
    targetRightVel = rightVel;
}

void VelocityController::setFeedforwardGain(float gain) {
    feedforwardGain = constrain(gain, 0.1, 10.0);
    TELEM_LOGF("Feedforward gain updated: %.2f PWM/(cm/s)", feedforwardGain);
}

void VelocityController::setDeadzone(float deadzone) {
    deadzonePWM = constrain(deadzone, 0.0, 100.0);
    TELEM_LOGF("Deadzone updated: %.0f PWM", deadzonePWM);
}

void VelocityController::setPIDGains(float kp, float ki, float kd) {
    leftPID.setGains(kp, ki, kd);
    rightPID.setGains(kp, ki, kd);
    TELEM_LOGF("PID gains updated: Kp=%.3f Ki=%.3f Kd=%.3f", kp, ki, kd);
}

void VelocityController::setPWMToVelocityPolynomial(const float* coeffs, int degree) {
    pwmToVelocityPoly.setCoefficients(coeffs, degree);
    TELEM_LOG("PWM->Velocity polynomial updated");
    for (int i = 0; i <= degree && i <= 5; i++) {
        TELEM_LOGF("  a%d = %.6f", i, coeffs[i]);
    }
}

void VelocityController::setVelocityToPWMPolynomial(const float* coeffs, int degree) {
    velocityToPWMPoly.setCoefficients(coeffs, degree);
    TELEM_LOG("Velocity->PWM polynomial updated");
    for (int i = 0; i <= degree && i <= 5; i++) {
        TELEM_LOGF("  a%d = %.6f", i, coeffs[i]);
    }
}

void VelocityController::enablePID(bool enable) {
    if (enable && !pidEnabled) {
        leftPID.reset();
        rightPID.reset();
    }
    pidEnabled = enable;
    TELEM_LOGF("PID %s", enable ? "enabled" : "disabled");
}

void VelocityController::getPIDGains(float& kp, float& ki, float& kd) const {
    kp = leftPID.getKp();
    ki = leftPID.getKi();
    kd = leftPID.getKd();
}

float VelocityController::velocityToPWM(float velocity) {
    if (abs(velocity) < 0.5) {
        return 0.0; 
    }
    
    float sign = (velocity >= 0) ? 1.0 : -1.0;
    float absVelocity = abs(velocity);
    float pwm;
    
    if (usePolynomialMapping) {
        pwm = velocityToPWMPoly.evaluate(absVelocity);
    } else {
        pwm = deadzonePWM + feedforwardGain * absVelocity;
    }
    
    pwm = constrain(pwm, 0.0, 255.0);
    
    return sign * pwm;
}

void VelocityController::update() {
    leftPWM = velocityToPWM(targetLeftVel);
    rightPWM = velocityToPWM(targetRightVel);
    
    if (pidEnabled && leftEncoder && rightEncoder) {
        float leftCorrection = leftPID.compute(targetLeftVel, leftEncoder->getVelocity());
        float rightCorrection = rightPID.compute(targetRightVel, rightEncoder->getVelocity());
        
        leftPWM += leftCorrection;
        rightPWM += rightCorrection;
        
        leftPWM = constrain(leftPWM, -255.0, 255.0);
        rightPWM = constrain(rightPWM, -255.0, 255.0);
    }
    
    driveController.setLeftMotorPower(leftPWM / 255.0);
    driveController.setRightMotorPower(rightPWM / 255.0);
    
    if (leftEncoder && rightEncoder) {
        leftVelError = targetLeftVel - leftEncoder->getVelocity();
        rightVelError = targetRightVel - rightEncoder->getVelocity();
    }
}
