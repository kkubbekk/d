#pragma once

#include <driver/gpio.h>
#include <driver/timer.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "tmc.hpp"
#include <cmath>

#define MOTOR_ENABLE GPIO_NUM_19
#define MOTOR_STEP GPIO_NUM_18
#define MOTOR_DIR GPIO_NUM_5

//rtos


//moptor parameters
#define ISR_STARTING_COOLDOWN 1000
#define STEPS_PER_REVOLUTION 200 //defualtowa wartosc bez mikrokrokow
#define MOTOR_ANGLE_MIN 0
#define MOTOR_ANGLE_MAX 360


//bo mama jakas stara werjse xD
#define TIMER_GROUP_0 (timer_group_t)0 
#define TIMER_NUM_0 (timer_idx_t)0

//pozniej wydziele klase tmc moze
class STEPPER_MOTOR
{
    portMUX_TYPE timer_mux = portMUX_INITIALIZER_UNLOCKED;
    const char * TAG = "MOTOR";
    volatile std::int32_t total_steps{0};
    volatile std::int32_t target_steps{0};
    volatile bool dir_forward{true};
    volatile bool step_state;
    uint16_t microsteps{8};
    
    void motor_isr_handler(void *arg);

    static void IRAM_ATTR tim_isr_trampoline(void *arg);
   

    public:
     TMC tmc_2209;
    
     void motor_init();
    
    void start_motor();


    float get_current_angle();
  

    float get_normalized_angle();

  
    

    void motor_thread();

    void start_rtos_thread(uint32_t stack_size, uint8_t priority);
    

     void move_to_angle(float angle_target);


    void stop_motor();
   
     void set_microsteps(uint16_t ms);
 
    


  


};