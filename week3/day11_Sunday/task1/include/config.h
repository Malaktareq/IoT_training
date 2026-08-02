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
 constexpr uint8_t  CON_LED_PIN = 17; 
// constexpr uint8_t  OVERCURRENT_LED_PIN = 16;

constexpr uint8_t  IR_PIN = 5;
// WiFi
#define WIFI_SSID "CYBER_EXT"
#define WIFI_PASSWORD "cyberap2025"


// MQTT Broker
#define MQTT_SERVER "broker.emqx.io"
#define MQTT_PORT 1883
#endif // CONFIG_H