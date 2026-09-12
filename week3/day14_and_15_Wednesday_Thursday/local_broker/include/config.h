#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

constexpr uint8_t SCL_PIN = 22;
constexpr uint8_t SDA_PIN = 21;
constexpr uint8_t EN_PIN = 18;
constexpr uint8_t IN2_PIN = 3;
constexpr uint8_t IN1_PIN = 19;

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define MQTT_BROKER "YOUR_MQTT_BROKER_IP"
const uint16_t MQTT_PORT = 1883;

#endif 