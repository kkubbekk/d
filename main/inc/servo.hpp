#pragma once

#include "driver/ledc.h"
#define LEDC_OUTPUT_IO GPIO_NUM_13
#define BOTTOM_PULSE_WIDTH 500
#define TOP_PULSE_WIDTH 2400
#define SERVO_ROTATIONAL_RANGE 180
#include "iot_servo.h"
#include "Iaxis.hpp"
class Servo : public Iaxis
{
    const char *servo_tag = "SERVO";
    float servo_angle{0.0f};
    servo_handle_t servo;

public:
    Servo()
    {
        servo_config_t servo_cfg = SERVO_CONFIG_DEFAULT(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, LEDC_CHANNEL_0, LEDC_OUTPUT_IO);

        iot_servo_new(&servo_cfg, &servo);

        move_to_angle(90.0f);
    }

    void move_to_angle(float angle) override
    {

        if (angle > 160.0f)
            angle = 160.0f;
        if (angle < 20.0f)
            angle = 20.0f;

        iot_servo_write_angle(servo, angle);

        iot_servo_read_angle(servo, &servo_angle);
    }

    virtual void move_by_delta(float delta) override
    {
        move_to_angle(servo_angle + delta);

        // ESP_LOGI(servo_tag,"obrot  o delta %f.2:",delta_angle);
    }

    virtual bool is_stopped() override
    {
        return true;
    }
};