
#include "header.h"
static const char *OUTPUT_TAG = "OUTPUT_TASK";
static const char *SENSOR_TAG = "SENSOR_TASK";
int adc_raw;
int ir_state;

adc_oneshot_unit_handle_t adc1_handle;

void pwm_setup()
{
    ledc_timer_config_t pwm_timer ={    
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_12_BIT,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    
    ledc_timer_config(&pwm_timer);
    ledc_channel_config_t pwm_channel = {
        .gpio_num = PWM_LED_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&pwm_channel);
}

void setup() 
{
    gpio_reset_pin(IR_GPIO);
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(IR_GPIO, GPIO_MODE_INPUT);

    adc_oneshot_unit_init_cfg_t init_config = {.unit_id = ADC_UNIT_1};
    adc_oneshot_new_unit(&init_config, &adc1_handle);
    adc_oneshot_chan_cfg_t config = {.bitwidth = ADC_BITWIDTH_DEFAULT,.atten = ADC_ATTEN_DB_12,};
    adc_oneshot_config_channel(adc1_handle, POT_CHANNEL, &config);
    pwm_setup();
}

void sensor_task(void *pvParameters)
{
  
    esp_err_t ret;

    while (1)
    {
        ret = adc_oneshot_read(adc1_handle, POT_CHANNEL, &adc_raw);
        if (ret == ESP_OK)
            ESP_LOGI(SENSOR_TAG, "Potentiometer raw value: %d", adc_raw);
        else 
            ESP_LOGE(SENSOR_TAG, "ADC read failed");
        ir_state = gpio_get_level(IR_GPIO);
        ESP_LOGI(SENSOR_TAG, "IR sensor state: %d", ir_state);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void output_task(void *pvParameters)
{
    while (1)
    {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, adc_raw);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0); // Update the duty cycle
        gpio_set_level(BLINK_GPIO, !ir_state); // Set GPIO level based on IR sensor state
        ESP_LOGI(OUTPUT_TAG, "PWM duty: %d, LED state: %d", adc_raw, !ir_state);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void app_main()
{
    setup();
    xTaskCreate(sensor_task, "Sensor Task", 2048, NULL, 1, NULL);
    xTaskCreate(output_task, "Output Task", 2560, NULL, 2, NULL);
}

