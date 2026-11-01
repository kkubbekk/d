
#include "ble.hpp"
#include "driver/gpio.h"
#include "stepper_motor.hpp"
#include <math.h>

#include "my_events.hpp"
//

ESP_EVENT_DEFINE_BASE(MOTOR_EVENT_BASE);


NavigationData navigation_data;

    // int16_t microsteps = 8;
extern "C" void app_main(void) {
    

     STEPPER_MOTOR motor_test;
    
    // motor_test.gpio_setup();
   
    motor_test.motor_init();
    motor_test.start_motor();

    motor_test.tmc_2209.tmc_init();

    motor_test.tmc_2209.set_current(50, 10);

   
    motor_test.tmc_2209.set_microsteps(8);


    motor_test.tmc_2209.set_stallguard_activation_val(50);


    motor_test.tmc_2209.set_stall_guard_vel_starting_point();

   
    motor_test.start_rtos_thread(4096,3); 



            esp_event_loop_args_t ble_yolo_event  = {
            .queue_size = (int32_t)10, //niewiem co wpisac bo moga tu rozne dane wchodzic naprykad o gps itp albo mozna zrobic tak ze ten event sie wykona tylko
            // jak przydjzie informacja o pozyji,
            .task_name = "motor_event_task",
            .task_priority = 5,            
            .task_stack_size = 2048,                
            .task_core_id =  1, 
        };

    
        esp_event_loop_handle_t loop_handle;

        esp_event_loop_create(&ble_yolo_event, &loop_handle);

        esp_event_handler_register_with(loop_handle, MOTOR_EVENT_BASE, YOLO_CMND_EVENT, &STEPPER_MOTOR::event_handler, &motor_test);

     BLE dupka(loop_handle,"rafal_cwel", 0x1234,0x1236,0x1235);   
         dupka.ble_init();




    // motor_test.tmc2209_write_cmnd(0x10, 0x00041008);
    const char* main_tag = "MAIN";
        for(;;)
    {

        // uint16_t prunt = motor_test.tmc_2209.read_current_val();

        // ESP_LOGI(main_tag,"prunt: %d",prunt);
        vTaskDelay(1000);   
        
     
   
    }
      
    }

