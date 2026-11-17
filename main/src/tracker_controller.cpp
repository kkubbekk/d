#include "tracker_controller.hpp"



    void TRACKER::home_initialization()
    {

    }

    void TRACKER::motor_thread_trampoline(void * arg)
    {
        TRACKER * motor_thread = static_cast<TRACKER *>(arg);

        motor_thread->stepper_motor_thread_loop();

        return;
    }

      void TRACKER::start_motor_rtos_thread()
      {
        xTaskCreatePinnedToCore(motor_thread_trampoline,"TASK MOTUR",MOTOR_TASK_STACK_SIZE,this,5,&MOTOR_TASK_HANDLE,0);
      }

      void TRACKER::stepper_motor_thread_loop()
      {
        Motor.motor_init(16, 50, 70);

        float human_angle = 0.0f;
        float step_deg = 1.5f; 

        for(;;)
        {
           
            // human_angle += step_deg;
            // if (human_angle >= 360.0f) {
            //     human_angle = 360.0f;
            //     step_deg = -1.5f; // Zawrot w lewo!
            // } else if (human_angle <= 0.0f) {
            //     human_angle = 0.0f;
            //     step_deg = 1.5f;  // Zawrot w prawo!
            // }

            Motor.move_to_angle(180);

             Motor.move_to_angle(180);
              Motor.move_to_angle(180);
            vTaskDelay(pdMS_TO_TICKS(1000));
            Motor.move_to_angle(360);
                 Motor.move_to_angle(360);
                      Motor.move_to_angle(360);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
      }