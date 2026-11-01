
#include "stepper_motor.hpp"


extern  QueueHandle_t cmndQueue;


 void STEPPER_MOTOR::motor_thread()
 {
    float angle_buf;

    

    for(;;)
    {


        if (xQueueReceive(cmndQueue, &angle_buf, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Received angle %.2f", angle_buf);
            move_to_angle(angle_buf);
        }

     

    }
}
void IRAM_ATTR STEPPER_MOTOR::motor_isr_handler(void *arg)
{
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_NUM_0);

    if (ramp == STOP) {
        is_motor_stopped = true;
        return;
    }

   
    gpio_set_level(MOTOR_STEP, 1);
    

    __asm__ __volatile__("nop; nop; nop; nop; nop; nop;");
    
    if (dir_forward) total_steps++;
    else total_steps--;
    
    gpio_set_level(MOTOR_STEP, 0);

    std::int32_t new_step_delay;

    switch (ramp) {
        case ACCEL:
            step_count++;
            accel_count++;
            
             
            new_step_delay = step_delay - ((2 * step_delay) + rest) / (4 * accel_count + 1);
            rest = ((2 * step_delay) + rest) % (4 * accel_count + 1);
            step_delay = new_step_delay;

            if (step_count >= decel_start) {
                accel_count = decel_val; 
                ramp = DECEL;
            } else if (step_delay <= min_delay) {
                rest = 0;
                step_delay = min_delay;
                ramp = RUN;
            }
            break;

        case RUN:
            step_count++;
            if (step_count >= decel_start) {
                accel_count = decel_val;
                ramp = DECEL;
            }
            break;

        case DECEL:
            step_count++;
            accel_count++; 
            
            new_step_delay = step_delay - ((2 * step_delay) + rest) / (4 * accel_count + 1);
            rest = ((2 * step_delay) + rest) % (4 * accel_count + 1);
            step_delay = new_step_delay;

            if (accel_count >= 0) {
                ramp = STOP;
            }
            break;

        case STOP:
            break;
    }

  
    timer_group_set_alarm_value_in_isr(TIMER_GROUP_0, TIMER_NUM_0, step_delay);
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_NUM_0);
}


 void  STEPPER_MOTOR::event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
    
        STEPPER_MOTOR* self = static_cast<STEPPER_MOTOR*>(handler_args);
        
        if (id == YOLO_CMND_EVENT) {
            float new_angle = *static_cast<float*>(event_data);
            // Wrzucamy do prywatnej kolejki
            xQueueOverwrite(self->cmndQueue, &new_angle); 
        }
    }


 void IRAM_ATTR STEPPER_MOTOR::tim_isr_trampoline(void *arg)
    {
        STEPPER_MOTOR* motor = static_cast<STEPPER_MOTOR*>(arg);

        motor->motor_isr_handler(arg);
    } 
    void motor_thread_wrapper(void* argument)
    {
        STEPPER_MOTOR* motor = static_cast<STEPPER_MOTOR *>(argument);

        motor->motor_thread();

        // ESP_LO`GE(TAG,"WYPIERDOLILO THREADA OD MOTURA");
    
        vTaskDelete(NULL); 


    }
 
       void STEPPER_MOTOR::motor_init()
    {
        //gpio na conf
        gpio_config_t gpio_motor_config = {
            .pin_bit_mask = (1ULL<< MOTOR_ENABLE) | (1ULL<< MOTOR_STEP) | (1ULL << MOTOR_DIR),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&gpio_motor_config);

        gpio_set_level(MOTOR_ENABLE, 0);


        timer_config_t tim_motor_config = {
            .alarm_en = TIMER_ALARM_EN,
            .counter_en = TIMER_START,
            .intr_type = TIMER_INTR_LEVEL,
            .counter_dir = TIMER_COUNT_UP,
            .auto_reload = TIMER_AUTORELOAD_EN,
            .clk_src = TIMER_SRC_CLK_DEFAULT, 
            .divider = 80
        };

        timer_init(TIMER_GROUP_0, TIMER_NUM_0, &tim_motor_config);
        timer_set_counter_value(TIMER_GROUP_0, TIMER_NUM_0, 0);
        timer_set_alarm_value(TIMER_GROUP_0, TIMER_NUM_0, ISR_STARTING_COOLDOWN);
        timer_isr_register(TIMER_GROUP_0, TIMER_NUM_0, &tim_isr_trampoline, this, ESP_INTR_FLAG_IRAM, NULL);
        timer_enable_intr(TIMER_GROUP_0, TIMER_NUM_0);
    }
    

     void STEPPER_MOTOR::start_motor()
{

    gpio_set_level(MOTOR_ENABLE, 0);

    step_state = 0;

    gpio_set_level(MOTOR_STEP, step_state); 

    timer_pause(TIMER_GROUP_0, TIMER_NUM_0);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_NUM_0, 0);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_NUM_0, ISR_STARTING_COOLDOWN);
    timer_set_alarm(TIMER_GROUP_0, TIMER_NUM_0, TIMER_ALARM_EN);
    timer_start(TIMER_GROUP_0, TIMER_NUM_0);
}

   
    float STEPPER_MOTOR::get_current_angle()
    {
        return total_steps* 360.0f/(STEPS_PER_REVOLUTION*microsteps);
    }

     float STEPPER_MOTOR::get_normalized_angle()
    {
        float raw_angle = get_current_angle();

        float normalized = fmodf(raw_angle,360.0f);

        if(normalized < 0.0f)
        {
            normalized +=360.0f;
        }
        return normalized;

    }
    void STEPPER_MOTOR::start_rtos_thread(uint32_t stack_size, uint8_t priority)
    {
        xTaskCreatePinnedToCore(
            motor_thread_wrapper,
            "task do silnika",
            stack_size,
            this,
            priority,
            NULL,
            1 
        );
    }
    void STEPPER_MOTOR::move_to_angle(float angle_target)
    {
    

       angle_target = fmodf(angle_target, 360.0f);
    if (angle_target < 0.0f) angle_target += 360.0f;

    
    float current_angle = get_current_angle();
    

    float normalized_current_angle = fmodf(current_angle,360.f);
    
    if(normalized_current_angle < 0.0f) normalized_current_angle +=360.f;

    float diff = angle_target - normalized_current_angle;
    if (diff > 180.0f) diff -= 360.0f;
    if (diff < -180.0f) diff += 360.0f;

    float steps_per_degree = (STEPS_PER_REVOLUTION * microsteps) / 360.0f;
    int32_t steps_to_add = static_cast<int32_t>(roundf(diff * steps_per_degree));

    if (steps_to_add == 0) return;

    portENTER_CRITICAL(&timer_mux);
    if (steps_to_add > 0) {
        dir_forward = true;
        gpio_set_level(MOTOR_DIR, 1);
    } else {
        dir_forward = false;
        gpio_set_level(MOTOR_DIR, 0);
    }

    int32_t steps_to_move = abs(steps_to_add);

  
    min_delay = static_cast<int32_t>(ft / target_speed);

    step_delay = static_cast<int32_t>(0.676f * ft * sqrtf(2.0f / accel));

 
    int32_t max_s_lim = static_cast<int32_t>((target_speed * target_speed) / (2.0f * accel));
    
    int32_t accel_lim = steps_to_move / 2;

    if (max_s_lim < accel_lim) {
     
        decel_start = steps_to_move - max_s_lim;
        decel_val = -max_s_lim;
    } else {
  
        decel_start = accel_lim;
        decel_val = -accel_lim;
    }


    step_count = 0;
    accel_count = 0;
    rest = 0;
    ramp = (steps_to_move == 1) ? DECEL : ACCEL;
    is_motor_stopped = false;

    // Start timera
    timer_set_counter_value(TIMER_GROUP_0, TIMER_NUM_0, 0);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_NUM_0, step_delay);
    timer_set_alarm(TIMER_GROUP_0, TIMER_NUM_0, TIMER_ALARM_EN);
    timer_start(TIMER_GROUP_0, TIMER_NUM_0);
    
    portEXIT_CRITICAL(&timer_mux);
}

 


    void STEPPER_MOTOR::stop_motor()
    {
        timer_pause(TIMER_GROUP_0,TIMER_NUM_0);

        timer_set_alarm(TIMER_GROUP_0, TIMER_NUM_0, TIMER_ALARM_DIS);

        gpio_set_level(MOTOR_ENABLE,0);
    }

     void STEPPER_MOTOR::set_microsteps(uint16_t ms)
    {
        portENTER_CRITICAL(&timer_mux);
       
        

        if(tmc_2209.set_microsteps(ms)!=0)
        {
            microsteps = ms;
             ESP_LOGI(TAG, "Mikrokroki zmienione na: %d", ms);
        } 
        portEXIT_CRITICAL(&timer_mux);

        
       
    }

