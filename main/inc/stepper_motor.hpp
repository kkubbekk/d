#pragma once

#include <driver/gpio.h>
#include <driver/timer.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "tmc.hpp"
#include <cmath>
#include "esp_event.h"
#include "my_events.hpp"

#define MOTOR_ENABLE GPIO_NUM_19
#define MOTOR_STEP GPIO_NUM_18
#define MOTOR_DIR GPIO_NUM_5

//mottor

typedef enum {
    STOP = 0,
    ACCEL,
    RUN,
    DECEL
} ramp_state_t;

enum class MotorState : uint8_t {
    WAIT_FOR_HOME,
    INITIALIZED,
    IDLE,
    TRACKING,
    ERROR
};


//moptor parameters
#define ISR_STARTING_COOLDOWN 1000
#define STEPS_PER_REVOLUTION 200 //defualtowa wartosc bez mikrokrokow
#define MOTOR_ANGLE_MIN 0
#define MOTOR_ANGLE_MAX 360
#define target_speed 4000.0f // max predkosc kroki/s
#define accel 2500.0f //przyspieszenie kroki/s^2
#define ft  1000000.0f // czestotliwosc przerwan
  


//bo mama jakas stara werjse xD
#define TIMER_GROUP_0 (timer_group_t)0 
#define TIMER_NUM_0 (timer_idx_t)0

//pozniej wydziele klase tmc moze
class STEPPER_MOTOR
{
    
    MotorState state_motor{MotorState::WAIT_FOR_HOME};
    QueueHandle_t cmndQueue;
    volatile ramp_state_t ramp{STOP};
    portMUX_TYPE timer_mux = portMUX_INITIALIZER_UNLOCKED;
    const char * TAG = "MOTOR";
    volatile std::int32_t total_steps{0};
    volatile std::int32_t target_steps{0};
    volatile bool dir_forward{true};
    volatile bool is_motor_stopped{true};
    volatile bool step_state; 
    volatile std::int32_t step_delay{0};    
    volatile std::int32_t min_delay{0};     
    volatile std::int32_t accel_count{0};   
    volatile std::int32_t decel_val{0};     
    volatile std::int32_t step_count{0};   
    volatile std::int32_t decel_start{0};   
    volatile std::int32_t rest{0};

    uint16_t microsteps{8};

    
    void motor_isr_handler(void *arg);

    static void IRAM_ATTR tim_isr_trampoline(void *arg);

    void send_angle_command(float target_angle) {
    xQueueOverwrite(cmndQueue, &target_angle);
    }
  
   

   
    // void calculate_motor_ramp_cldwn()
    // {
        
    //     if(target_steps < )
    
    // }

    public:
     TMC tmc_2209;

    static void event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);

     void motor_init();
    
    void start_motor();

     STEPPER_MOTOR() {
        cmndQueue = xQueueCreate(1, sizeof(float));
    }


    float get_current_angle();
  

    float get_normalized_angle();

   
      MotorState get_state()
    {
        return state_motor;
    }

    void motor_thread();

    void start_rtos_thread(uint32_t stack_size, uint8_t priority);
    

    void move_to_angle(float angle_target);


    void stop_motor();
   
    void set_microsteps(uint16_t ms);
 
    


  


};