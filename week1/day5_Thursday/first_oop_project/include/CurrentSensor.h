#ifndef CURRENT_SENSOR_H
#define CURRENT_SENSOR_H

#include "config.h"

void CurrentSensor_Init(int8_t pin, float sensitivity);
float CurrentSensor_ReadAmps();
bool CurrentSensor_IsOvercurrent(float threshold);

#endif