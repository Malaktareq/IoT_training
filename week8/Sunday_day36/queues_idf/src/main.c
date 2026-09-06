#include "header.h"

static QueueHandle_t sensor_queue;

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
    sensor_message_t sensor_data;
    esp_err_t ret;

    while (1)
    {
        ret = adc_oneshot_read(adc1_handle, POT_CHANNEL, &sensor_data.adc_raw);
        if (ret == ESP_OK)
            ESP_LOGI(SENSOR_TAG, "Potentiometer raw value: %d", sensor_data.adc_raw);
        else 
            ESP_LOGE(SENSOR_TAG, "ADC read failed");
        sensor_data.ir_state = gpio_get_level(IR_GPIO);
        ESP_LOGI(SENSOR_TAG, "IR sensor state: %d", sensor_data.ir_state);
        if(xQueueSend(sensor_queue, &sensor_data, pdMS_TO_TICKS(100)) != pdPASS)
            ESP_LOGE(SENSOR_TAG, "Failed to send sensor data to queue");
        else
            ESP_LOGI(SENSOR_TAG, "Sensor data sent to queue: adc_raw=%d, ir_state=%d", sensor_data.adc_raw, sensor_data.ir_state);
        vTaskDelay(pdMS_TO_TICKS(SENSOR_INTERVAL_MS));
    }
}

void output_task(void *pvParameters)
{
    sensor_message_t received_data;
    while (1)
    {
        if (xQueueReceive(sensor_queue, &received_data, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGI(OUTPUT_TAG, "Received sensor data: adc_raw=%d, ir_state=%d", received_data.adc_raw, received_data.ir_state);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, received_data.adc_raw);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            gpio_set_level(BLINK_GPIO, !received_data.ir_state);
            ESP_LOGI(OUTPUT_TAG, "PWM duty set to: %d, LED state: %d", received_data.adc_raw, !received_data.ir_state);
            vTaskDelay(pdMS_TO_TICKS(OUTPUT_PROCESS_TIME_MS));
        }
        else
            ESP_LOGE(OUTPUT_TAG, "Failed to receive sensor data from queue");
    }
}

void app_main() 
{
    setup();
    sensor_queue = xQueueCreate(QUEUE_DEPTH,sizeof(sensor_message_t));
    if (sensor_queue == NULL)
    {
        ESP_LOGE("MAIN", "Failed to create sensor queue");
        return;
    }
    ESP_LOGI("MAIN","Sensor queue created with depth %d",QUEUE_DEPTH);
    xTaskCreate(sensor_task, SENSOR_TASK, 2048, NULL, 1, NULL);
    xTaskCreate(output_task, OUTPUT_TASK, 2560, NULL, 2, NULL);
}