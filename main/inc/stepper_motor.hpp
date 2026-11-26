#pragma once

#include <driver/gpio.h>
#include <driver/timer.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "tmc.hpp"
#include <cmath>
#include "Iaxis.hpp"
#include "esp_timer.h"

#define MOTOR_ENABLE GPIO_NUM_19
#define MOTOR_STEP GPIO_NUM_18
#define MOTOR_DIR GPIO_NUM_5
#define KP 0.3
#define KD 1

// mottor

// moptor parameters
#define ISR_STARTING_COOLDOWN 1000
#define STEPS_PER_REVOLUTION 200 // defualtowa wartosc bez mikrokrokow
#define MOTOR_ANGLE_MIN 0
#define MOTOR_ANGLE_MAX 360
#define IDLE_CLOCK_ISR_SPEED 500
#define ft 1000000.0f // czestotliwosc przerwan

// bo mama jakas stara werjse xD
#define TIMER_GROUP_0 (timer_group_t)0
#define TIMER_NUM_0 (timer_idx_t)0

// pozniej wydziele klase tmc moze
class STEPPER_MOTOR : public Iaxis
{

    TMC tmc_2209;
    QueueHandle_t cmndQueue;
    // volatile ramp_state_t ramp{STOP};
    portMUX_TYPE timer_mux = portMUX_INITIALIZER_UNLOCKED;
    const char *TAG = "MOTOR";
    volatile std::int32_t total_steps{0};
    volatile std::int32_t target_steps{0};
    volatile bool dir_forward{true};
    volatile bool is_motor_stopped{true};
    volatile bool step_state;
    volatile int32_t last_step_count_pd{};
    int64_t last_pd_time_us{};
    


    uint16_t microsteps{16};

    void motor_isr_handler(void *arg);

    static void IRAM_ATTR tim_isr_trampoline(void *arg);

    void send_angle_command(float target_angle)
    {
        xQueueOverwrite(cmndQueue, &target_angle);
    }

    // void calculate_motor_ramp_cldwn()
    // {

    //     if(target_steps < )

    // }

public:
    bool is_stopped() override;
    void set_current_pos_as_zero();

    void set_current_pos_as_azimuth(float azimuth_deg) override;

    void motor_init(std::uint16_t microsteps, std::uint16_t hold_current, std::uint16_t irun_current, std::uint8_t stallguar_act_point = 100);

    void start_motor();

    STEPPER_MOTOR()
    {
    }

    float get_current_angle();

    float get_normalized_angle();

    void motor_thread();

    void move_by_delta(float delta) override;

    void move_to_angle(float angle) override;

    void stop_motor();

    void set_microsteps(uint16_t ms);

    void set_current(std::uint16_t hold_current_percent, std::uint16_t run_current_percent);
};