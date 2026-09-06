#ifndef HEADER_H
#define HEADER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include <stdio.h>

#define QUEUE_DEPTH 1
#define BLINK_GPIO 18
#define IR_GPIO 19
#define POT_CHANNEL ADC_CHANNEL_6
#define PWM_LED_GPIO 21
#define SENSOR_INTERVAL_MS 100
#define OUTPUT_PROCESS_TIME_MS 500
#define RECEIVE_TIMEOUT_MS 1000

static const char *OUTPUT_TASK= "OUTPUT_TASK";
static const char *SENSOR_TASK = "SENSOR_TASK";
static const char *OUTPUT_TAG = "OUTPUT";
static const char *SENSOR_TAG = "SENSOR";
typedef struct
{
    int adc_raw;
    int ir_state;
} sensor_message_t;

#endif