#include "BatteryMonitor.h"
#include "../network/Telemetry.h"

BatteryMonitor::BatteryMonitor(uint8_t pin, float multiplier) 
    : adcPin(pin), voltageMultiplier(multiplier) {}

void BatteryMonitor::begin() {
    pinMode(adcPin, INPUT);
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);  // Set ADC to read up to 3.3V
    
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &adcChars);
    
    TELEM_LOG("✓ Battery monitor initialized (readings may be erratic)");
}

float BatteryMonitor::getVoltage() {
    int adcValue = analogRead(adcPin);
    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(adcValue, &adcChars);
    return (voltage_mv / 1000.0) * voltageMultiplier;
}
