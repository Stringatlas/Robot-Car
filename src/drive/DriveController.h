#ifndef DRIVECONTROLLER_H
#define DRIVECONTROLLER_H

#include <Arduino.h>

class DriveController {
public:
    DriveController();
    void begin();
    void setPowerControl(float forward, float turn);
    void setLeftMotorPower(float power);   // -1.0 to 1.0
    void setRightMotorPower(float power);  // -1.0 to 1.0
    int getLastLeftPWM() const { return lastLeftPWM; }
    int getLastRightPWM() const { return lastRightPWM; }

private:
    int lastLeftPWM;
    int lastRightPWM;
};

#endif
