#ifndef IDF_FIRST_PROJECT_H
#define IDF_FIRST_PROJECT_H

#include <stdio.h>
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/ledc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define BLINK_GPIO 18
#define IR_GPIO 19
#define POT_CHANNEL ADC_CHANNEL_6
#define PWM_LED_GPIO 21


#endif // IDF_FIRST_PROJECT_H