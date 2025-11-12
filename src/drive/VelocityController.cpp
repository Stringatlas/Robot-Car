#include "VelocityController.h"
#include "./DriveController.h"
#include "../network/Telemetry.h"

// External reference to the global drive controller
extern DriveController driveController;

VelocityController::VelocityController() 
    : leftEncoder(nullptr), rightEncoder(nullptr),
      targetLeftVel(0), targetRightVel(0),
      feedforwardGain(3),
      deadzonePWM(60),
      pwmToVelocityPoly(),    // Default identity
      velocityToPWMPoly(),    // Default identity
      usePolynomialMapping(false),
      leftPID(0, 0, 0), rightPID(0, 0, 0), pidEnabled(false),
      leftPWM(0), rightPWM(0),
      leftVelError(0), rightVelError(0) {
    // Set PID output limits to match PWM correction range
    leftPID.setOutputLimits(-100, 100);
    rightPID.setOutputLimits(-100, 100);
}

void VelocityController::begin() {
    TELEM_LOG("✓ Velocity controller initialized");
    TELEM_LOGF("  Feedforward gain: %.2f PWM/(cm/s)", feedforwardGain);
    TELEM_LOGF("  Deadzone: %.0f PWM", deadzonePWM);
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
        // Reset PIDs when enabling
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
        return 0.0;  // Stop if velocity is very small
    }
    
    float sign = (velocity >= 0) ? 1.0 : -1.0;
    float absVelocity = abs(velocity);
    float pwm;
    
    if (usePolynomialMapping) {
        // Use polynomial mapping: velocity -> PWM
        pwm = velocityToPWMPoly.evaluate(absVelocity);
    } else {
        // Legacy feedforward model: PWM = deadzone + gain * velocity
        pwm = deadzonePWM + feedforwardGain * absVelocity;
    }
    
    // Clamp to valid range
    pwm = constrain(pwm, 0.0, 255.0);
    
    return sign * pwm;
}

void VelocityController::update() {
    // Compute base PWM from desired velocities using feedforward
    leftPWM = velocityToPWM(targetLeftVel);
    rightPWM = velocityToPWM(targetRightVel);
    
    // Add PID correction if enabled
    if (pidEnabled && leftEncoder && rightEncoder) {
        float leftCorrection = leftPID.compute(targetLeftVel, leftEncoder->getVelocity());
        float rightCorrection = rightPID.compute(targetRightVel, rightEncoder->getVelocity());
        
        leftPWM += leftCorrection;
        rightPWM += rightCorrection;
        
        // Clamp to valid range
        leftPWM = constrain(leftPWM, -255.0, 255.0);
        rightPWM = constrain(rightPWM, -255.0, 255.0);
    }
    
    // Apply to motors (convert to -1.0 to 1.0 range)
    driveController.setLeftMotorPower(leftPWM / 255.0);
    driveController.setRightMotorPower(rightPWM / 255.0);
    
    // Calculate velocity tracking errors (for tuning feedback)
    if (leftEncoder && rightEncoder) {
        leftVelError = targetLeftVel - leftEncoder->getVelocity();
        rightVelError = targetRightVel - rightEncoder->getVelocity();
    }
}
