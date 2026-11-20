#include "driver/ledc.h"
#define LEDC_OUTPUT_IO 13
#define BOTTOM_PULSE_WIDTH 500
#define TOP_PULSE_WIDTH 2400
#define SERVO_ROTATIONAL_RANGE 180
class Servo
{
    float servo_angle{90.0f};

public:
    void servo_init()
    {
        ledc_timer_config_t servo_conf =
            {
                .speed_mode = LEDC_HIGH_SPEED_MODE,
                .duty_resolution = LEDC_TIMER_14_BIT,
                .timer_num = LEDC_TIMER_0,
                .freq_hz = 50,
                .clk_cfg = LEDC_AUTO_CLK
            };

        ESP_ERROR_CHECK(ledc_timer_config(&servo_conf));

        ledc_channel_config_t ledc_channel =
            {
                .speed_mode = LEDC_HIGH_SPEED_MODE,
                .channel = LEDC_CHANNEL_0,
                .timer_sel = LEDC_TIMER_0,
                .gpio_num = LEDC_OUTPUT_IO,
                .duty = 0, // Set duty to 0%
                .hpoint = 0,
        };

        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    }

    void move_servo_to_angle(float angle)
    {
        float desired_duty = 500 + (angle/SERVO_ROTATIONAL_RANGE)*(2400-500);

        ledc_set_duty(desired_duty);

        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
    }
}