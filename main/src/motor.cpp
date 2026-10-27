
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

        vTaskDelay(10);

    }
}

void STEPPER_MOTOR:: motor_isr_handler(void *arg)
{
     timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_NUM_0);

    
    step_state = !step_state;
    gpio_set_level(MOTOR_STEP, step_state);

    if (step_state) 
    {
        if (total_steps < target_steps)
        {
            gpio_set_level(MOTOR_DIR, 1);
            dir_forward = true;
            total_steps++;
        }
        else if (total_steps > target_steps)
        {
            gpio_set_level(MOTOR_DIR, 0);
            dir_forward = false;
            total_steps--;
        }
  
    }

    if(total_steps != target_steps)
    {
        timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_NUM_0);
    }

}
      
    


 void IRAM_ATTR STEPPER_MOTOR::tim_isr_trampoline(void *arg)
    {
        STEPPER_MOTOR* motor = static_cast<STEPPER_MOTOR*>(arg);

        motor->motor_isr_handler(arg);
    } 
    static void motor_thread_wrapper(void* argument)
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

    portENTER_CRITICAL(&timer_mux);
    target_steps = total_steps + steps_to_add; 
    
    timer_set_alarm(TIMER_GROUP_0, TIMER_NUM_0, TIMER_ALARM_EN);
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

