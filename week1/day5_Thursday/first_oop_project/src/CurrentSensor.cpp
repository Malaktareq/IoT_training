#include "../include/CurrentSensor.h"

void CurrentSensor_Init(int8_t pin, float sensitivity) {

    analogSetPinAttenuation(pin, ADC_11db); // Set the attenuation for the ADC pin which is the default for ESP32, allowing for a wider voltage range (0-3.3V)
}

float CurrentSensor_ReadAmps()
{
    int sum;
    float averageVoltage;
    float voltage; 
    float current;

    sum = 0;
    for (int i = 0; i < 50; i++)
    {
        sum += analogRead(SENSOR_PIN);
        delay(5); // Small delay to allow for ADC stabilization
    }
    averageVoltage = sum / 50.0;
    voltage = (averageVoltage / 4095.0) * 3.3; // Convert ADC value to voltage (assuming 12-bit ADC and 3.3V reference)
    current = (voltage - CURRENT_SENSOR_OFFSET) / CURRENT_SENSOR_SCALE; // Convert voltage to current using the sensor's scale factor and offset
    return current;
}

bool CurrentSensor_IsOvercurrent(float threshold)
{
    float current = CurrentSensor_ReadAmps();
    return current > threshold;
}

