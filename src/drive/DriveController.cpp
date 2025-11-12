#include "DriveController.h"
#include "config.h"
#include "../network/Telemetry.h"

DriveController::DriveController() : lastLeftPWM(0), lastRightPWM(0) {}

void DriveController::begin() {
    // Configure motor pins
    pinMode(MOTOR_IN1, OUTPUT);
    pinMode(MOTOR_IN2, OUTPUT);
    pinMode(MOTOR_IN3, OUTPUT);
    pinMode(MOTOR_IN4, OUTPUT);
    
    // Configure PWM channels
    ledcSetup(MOTOR_LEFT_PWM_CHANNEL, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
    ledcSetup(MOTOR_RIGHT_PWM_CHANNEL, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
    ledcAttachPin(MOTOR_ENA, MOTOR_RIGHT_PWM_CHANNEL);
    ledcAttachPin(MOTOR_ENB, MOTOR_LEFT_PWM_CHANNEL);
    
    TELEM_LOG("✓ Drive controller initialized");
}

void DriveController::setLeftMotorPower(float power) {
    power = constrain(power, -1.0, 1.0);
    int pwm = abs(power) * 255;

    // Store for broadcasting (with sign)
    lastLeftPWM = (power >= 0) ? pwm : -pwm;
    
    if (power > 0.05) {
        digitalWrite(MOTOR_IN3, LOW);
        digitalWrite(MOTOR_IN4, HIGH);
        ledcWrite(MOTOR_LEFT_PWM_CHANNEL, pwm);
    } else if (power < -0.05) {
        digitalWrite(MOTOR_IN3, HIGH);
        digitalWrite(MOTOR_IN4, LOW);
        ledcWrite(MOTOR_LEFT_PWM_CHANNEL, pwm);
    } else {
        digitalWrite(MOTOR_IN3, LOW);
        digitalWrite(MOTOR_IN4, LOW);
        ledcWrite(MOTOR_LEFT_PWM_CHANNEL, 0);
    }
}

void DriveController::setRightMotorPower(float power) {
    power = constrain(power, -1.0, 1.0);
    int pwm = abs(power) * 255;
    
    // Store for broadcasting (with sign)
    lastRightPWM = (power >= 0) ? pwm : -pwm;
    
    if (power > 0.05) {
        digitalWrite(MOTOR_IN1, LOW);
        digitalWrite(MOTOR_IN2, HIGH);
        ledcWrite(MOTOR_RIGHT_PWM_CHANNEL, pwm);
    } else if (power < -0.05) {
        digitalWrite(MOTOR_IN1, HIGH);
        digitalWrite(MOTOR_IN2, LOW);
        ledcWrite(MOTOR_RIGHT_PWM_CHANNEL, pwm);
    } else {
        digitalWrite(MOTOR_IN1, LOW);
        digitalWrite(MOTOR_IN2, LOW);
        ledcWrite(MOTOR_RIGHT_PWM_CHANNEL, 0);
    }
}

void DriveController::setPowerControl(float forward, float turn) {
    // Differential drive with forward (-1 to 1) and turn (-1 to 1)
    // forward: -1.0 (backward) to 1.0 (forward)
    // turn: -1.0 (left) to 1.0 (right)
    
    // Basic differential steering formula
    float leftSpeed = forward + turn;
    float rightSpeed = forward - turn;
    
    // Normalize if either exceeds 1.0 to preserve turning ratio
    float maxSpeed = max(abs(leftSpeed), abs(rightSpeed));
    if (maxSpeed > 1.0) {
        leftSpeed /= maxSpeed;
        rightSpeed /= maxSpeed;
    }
    
    // Use the modular power functions
    setLeftMotorPower(leftSpeed);
    setRightMotorPower(rightSpeed);
    
    // Debug output
    TELEM_LOGF("🕹️ Fwd:%.2f Turn:%.2f -> L:%.2f(%d) R:%.2f(%d)", 
               forward, turn, leftSpeed, lastLeftPWM, rightSpeed, lastRightPWM);
}
