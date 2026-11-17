
#include "stepper_motor.hpp"




void IRAM_ATTR STEPPER_MOTOR::motor_isr_handler(void *arg)
{
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_NUM_0);

    if (ramp == STOP) {
        is_motor_stopped = true;
        return;
    }

   
    gpio_set_level(MOTOR_STEP, 1);
    

   // __asm__ __volatile__("nop; nop; nop; nop; nop; nop;"); // linijka z chata wydluzam impuls na high zeby tmc zalapalo
    
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
            if (accel_count >= 0) {
                ramp = STOP;
                is_motor_stopped = true;
                return;
            }
            
            new_step_delay = step_delay - ((2 * step_delay) + rest) / (4 * accel_count + 1);
            rest = ((2 * step_delay) + rest) % (4 * accel_count + 1);
            step_delay = new_step_delay;
            break;

        case STOP:
            return;
            break;
    }

  
    timer_group_set_alarm_value_in_isr(TIMER_GROUP_0, TIMER_NUM_0, step_delay);
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_NUM_0);
}





 void IRAM_ATTR STEPPER_MOTOR::tim_isr_trampoline(void *arg)
    {
        STEPPER_MOTOR* motor = static_cast<STEPPER_MOTOR*>(arg);

        motor->motor_isr_handler(arg);
    } 
  

 
     void STEPPER_MOTOR::motor_init(std::uint16_t microsteps,std::uint16_t hold_current,std::uint16_t irun_current,std::uint8_t stallguar_act_point)
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

        tmc_2209.tmc_init();

        set_microsteps(microsteps);

        set_current(hold_current,irun_current);

        start_motor();

    }




    void STEPPER_MOTOR::set_current(std::uint16_t hold_current_percent,std::uint16_t run_current_percent)
    {
        if(hold_current_percent > 100) hold_current_percent = 100;
     

        if(run_current_percent > 100) run_current_percent = 100;
    

        tmc_2209.set_current(run_current_percent,hold_current_percent);
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

    void STEPPER_MOTOR::move_to_angle(float angle_target)
    {
        angle_target = fmodf(angle_target, 360.0f);
        if (angle_target < 0.0f) angle_target += 360.0f;

        
        const int32_t pos_now = total_steps;

        float current_angle = pos_now * 360.0f / (STEPS_PER_REVOLUTION * microsteps);
        float normalized_current_angle = fmodf(current_angle, 360.0f);
        if (normalized_current_angle < 0.0f) normalized_current_angle += 360.0f;

        float diff = angle_target - normalized_current_angle;
        if (diff > 180.0f) diff -= 360.0f;
        if (diff < -180.0f) diff += 360.0f;

        float steps_per_degree = (STEPS_PER_REVOLUTION * microsteps) / 360.0f;
        int32_t steps_to_add = static_cast<int32_t>(roundf(diff * steps_per_degree));

        
        const int32_t max_s_lim = static_cast<int32_t>((target_speed * target_speed) / (2.0f * accel));

        
        auto plan_braking = [&](int32_t n_now, int32_t sc_now, int32_t R, bool cruising) {
            int32_t peak = (R + n_now) / 2;
            if (peak > max_s_lim) peak = max_s_lim;      
            if (cruising || peak < n_now) peak = n_now; 
            decel_val   = -peak;
            decel_start = sc_now + R - peak;             
        };

        portENTER_CRITICAL(&timer_mux);

       
        target_steps = pos_now + steps_to_add;
        const int32_t remaining = target_steps - total_steps; 
        const int32_t steps_to_move = abs(remaining);
        const bool want_forward = (remaining > 0);

        const int32_t n  = accel_count;
        const int32_t sc = step_count;

        if (ramp == STOP) {
          
            if (steps_to_move == 0) {
                portEXIT_CRITICAL(&timer_mux);
                return;
            }

            
            dir_forward = want_forward;
            gpio_set_level(MOTOR_DIR, want_forward ? 1 : 0);

            min_delay = static_cast<int32_t>(ft / target_speed);
            step_delay = static_cast<int32_t>(0.676f * ft * sqrtf(2.0f / accel));

            plan_braking(0, 0, steps_to_move, false);

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
        }
        else if (ramp == ACCEL || ramp == RUN) {
            const int32_t R = (want_forward == dir_forward) ? steps_to_move : 0;
            plan_braking(n, sc, R, ramp == RUN);
        }
        else if (ramp == DECEL) {
            // W DECEL accel_count (n) jest ujemne, a -n oznacza ile krokow potrzebujemy do zatrzymania.
            // Jesli cel znowu oddalil sie w kierunku jazdy (R > -n), przerywamy hamowanie i plynnie wracamy do ACCEL!
            const int32_t n_pos = -n;
            if (want_forward == dir_forward && steps_to_move > n_pos) {
                accel_count = n_pos;
                ramp = ACCEL;
                plan_braking(n_pos, sc, steps_to_move, false);
            }
        }
       
        portEXIT_CRITICAL(&timer_mux);
    }

void STEPPER_MOTOR::move_by_delta(float delta)
{
    move_to_angle(get_normalized_angle()+delta);
}

 


    void STEPPER_MOTOR::stop_motor()
    {
        timer_pause(TIMER_GROUP_0,TIMER_NUM_0);

        timer_set_alarm(TIMER_GROUP_0, TIMER_NUM_0, TIMER_ALARM_DIS);

        gpio_set_level(MOTOR_ENABLE,0);
    }

    void STEPPER_MOTOR::set_current_pos_as_zero()
    {
        portENTER_CRITICAL(&timer_mux);
        total_steps = 0;
        portEXIT_CRITICAL(&timer_mux);
    }


     void STEPPER_MOTOR::set_microsteps(uint16_t ms)
    {
        
        

        if(tmc_2209.set_microsteps(ms)!=0)
        {
            portENTER_CRITICAL(&timer_mux);
       
            microsteps = ms;

            portEXIT_CRITICAL(&timer_mux);
            
            ESP_LOGI(TAG, "Mikrokroki zmienione na: %d", ms);
        } 


        
       
    }

