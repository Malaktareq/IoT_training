#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <Arduino.h>
// 298N Motor Control Pins
constexpr uint8_t  SENSOR_PIN = 4;
constexpr uint8_t  MOTOR_SPEED_PIN1 = 8;
constexpr uint8_t  MOTOR_DIR_PIN2 = 3;

constexpr uint8_t  MOTOR_ENA = 18;
constexpr uint8_t  E_STOP_PIN = 6;
constexpr uint8_t  RUNNING_LED_PIN = 15;
constexpr uint8_t  STOP_LED_PIN = 7;
constexpr uint8_t  LOW_CURRENT_LED_PIN = 17; 
constexpr uint8_t  OVERCURRENT_LED_PIN = 16;

// Current Sensor Parameters
constexpr float CURRENT_THRESHOLD = 0.4; // Current threshold in Amperes
constexpr float CURRENT_SENSOR_SCALE = 0.1221; // Current sensor scale factor (V/A)
constexpr float CURRENT_SENSOR_OFFSET = 0.72; // Current sensor offset voltage (V)
#endif // CONFIG_H