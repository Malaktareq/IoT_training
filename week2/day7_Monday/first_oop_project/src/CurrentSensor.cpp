#include "../include/CurrentSensor.h"

static int8_t currentSensorPin = -1;

void CurrentSensor_Init(int8_t pin, float sensitivity) {
    currentSensorPin = pin;
    analogSetPinAttenuation(pin, ADC_11db); // Set the attenuation for the ADC pin which is the default for ESP32, allowing for a wider voltage range (0-3.3V)
}

float CurrentSensor_ReadAmps()
{
    int sum;
    float averageVoltage;
    float voltage; 
    float current;
    static float filteredCurrent = 0.0;

    sum = 0;
    for (int i = 0; i < 20; i++)
    {
        sum += analogRead(SENSOR_PIN);
        delayMicroseconds(5); // Small delay to allow for ADC stabilization
    }
    averageVoltage = sum / 20.0;
    voltage = (averageVoltage / 4095.0) * 3.3; // Convert ADC value to voltage (assuming 12-bit ADC and 3.3V reference)
    current = (voltage - CURRENT_SENSOR_OFFSET) / CURRENT_SENSOR_SCALE; // Convert voltage to current using the sensor's scale factor and offset
    filteredCurrent += alpha * (current - filteredCurrent); // Apply low-pass filter to smooth the current reading using Exponential Moving Average (EMA) method

    return filteredCurrent;
}

bool CurrentSensor_IsOvercurrent(float threshold, volatile SystemState &systemState)
{
    float current = CurrentSensor_ReadAmps();
    Serial.print("Current: ");
    Serial.print(current, 2); // Print current with 2 decimal places
    Serial.println(" A");

    if (current > threshold)
    {
        if (systemState != SystemState::EMERGENCY_STOP)
        {
            systemState = SystemState::FAULT_OVERCURRENT;
        }
        return true;
    }
    return false;
}

