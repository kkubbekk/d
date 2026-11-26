
#include "ble.hpp"
#include "driver/gpio.h"
#include "stepper_motor.hpp"
#include "tracker_controller.hpp"
#include "shared_data.hpp"
#include "servo.hpp"
#include <math.h>

const char *main_tag = "MAIN";

extern "C" void app_main(void)
{
    static SharedNavigationData shared_data;
    static BLE ble_module(shared_data, "hihi");
   

    static STEPPER_MOTOR motor_x;

    motor_x.motor_init(32, 50, 70);

    static Servo servo_y;

    static TRACKER tracker(shared_data, motor_x, servo_y);

    ble_module.ble_init();
    tracker.start_motor_rtos_thread();

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
