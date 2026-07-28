#ifndef CONFIG_H
#define CONFIG_H
#include <stdint.h>
#include <Arduino.h>

constexpr uint16_t BME280_I2C_ADDRESS = 0x76;
constexpr uint32_t SCL_PIN = 22;
constexpr uint32_t SDA_PIN = 21;

#endif