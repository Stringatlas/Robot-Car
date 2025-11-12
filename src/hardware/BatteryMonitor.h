#ifndef BATTERYMONITOR_H
#define BATTERYMONITOR_H

#include <Arduino.h>
#include <esp_adc_cal.h>

class BatteryMonitor {
public:
    BatteryMonitor(uint8_t pin, float multiplier);
    void begin();
    float getVoltage();

private:
    uint8_t adcPin;
    float voltageMultiplier;
    esp_adc_cal_characteristics_t adcChars;
};

#endif
