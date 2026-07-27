#ifndef CURRENT_SENSOR_H
#define CURRENT_SENSOR_H

#include "config.h"
#include "enum_class.h"

#define alpha 0.3 // Low-pass filter coefficient for smoothing current readings


void CurrentSensor_Init(int8_t pin, float sensitivity);
float CurrentSensor_ReadAmps();
bool CurrentSensor_IsOvercurrent(float threshold, volatile SystemState &systemState);

#endif