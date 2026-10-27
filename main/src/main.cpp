
#include "ble.hpp"
#include "driver/gpio.h"
#include "stepper_motor.hpp"
#include <math.h>

QueueHandle_t cmndQueue = xQueueCreate(1, sizeof(float));
    // int16_t microsteps = 8;
extern "C" void app_main(void) {
    


     BLE dupka("rafal_cwel", 0x1234,0x1236,0x1235);   
     STEPPER_MOTOR motor_test;
    dupka.ble_init();

    
    // motor_test.gpio_setup();
    uint8_t dane = 3;
    motor_test.motor_init();
    motor_test.start_motor();

    motor_test.tmc_2209.tmc_init();

    motor_test.tmc_2209.set_current(50, 10);

   
    motor_test.tmc_2209.set_microsteps(8);


    motor_test.tmc_2209.set_stallguard_activation_val(50);


    motor_test.tmc_2209.set_stall_guard_vel_starting_point();

     motor_test.start_rtos_thread(4096,3); 



    // motor_test.tmc2209_write_cmnd(0x10, 0x00041008);
    const char* main_tag = "MAIN";
        for(;;)
    {

        // uint16_t prunt = motor_test.tmc_2209.read_current_val();

        // ESP_LOGI(main_tag,"prunt: %d",prunt);
        vTaskDelay(1000);   
        
     
   
    }
      
    }



