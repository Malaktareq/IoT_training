#include "idf_first_project.h"

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

void app_main() 
{
    bool ir_state;
    int adc_raw; 

    setup();
    while(1) 
    {
        ir_state = gpio_get_level(IR_GPIO);
        adc_oneshot_read(adc1_handle, POT_CHANNEL, &adc_raw);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, adc_raw);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        
        printf("IR state: %d, Potentiometer raw value: %d\n", ir_state, adc_raw);
        if(ir_state == 0)
            gpio_set_level(BLINK_GPIO, 1);
        else
            gpio_set_level(BLINK_GPIO, 0);
    }
}
