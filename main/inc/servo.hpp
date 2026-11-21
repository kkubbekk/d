#include "driver/ledc.h"
#define LEDC_OUTPUT_IO GPIO_NUM_13
#define BOTTOM_PULSE_WIDTH 500
#define TOP_PULSE_WIDTH 2400
#define SERVO_ROTATIONAL_RANGE 180
#include "iot_servo.h"
class Servo
{
    const char * servo_tag = "SERVO";
    float servo_angle{90.0f};
     servo_handle_t servo;

public:
    void servo_init()
    {
       


        servo_config_t servo_cfg = SERVO_CONFIG_DEFAULT(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, LEDC_CHANNEL_0, LEDC_OUTPUT_IO);

        iot_servo_new(&servo_cfg,&servo);

        iot_servo_write_angle(servo,90.0);


    }

    void servo_move_angle(float angle)
    {
        iot_servo_read_angle(servo,&servo_angle);
        
        
        iot_servo_write_angle(servo, angle);
    }
    
    void move_servo_by_delta(float delta_angle)
    {
        iot_servo_read_angle(servo,&servo_angle);

        float new_angle = servo_angle + delta_angle;
        if(new_angle >160.0f) new_angle = 160.0f;
        if(new_angle < 20.0f) new_angle = 20.0f;

        iot_servo_write_angle(servo,new_angle);
        ESP_LOGI(servo_tag,"obrot  o delta %f.2:",delta_angle);

    }
};