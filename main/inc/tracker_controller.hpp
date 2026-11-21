
#pragma once

#include "stepper_motor.hpp"
#include "gps.hpp"
#include "shared_data.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define DEADZONE 2
#define MOTOR_TASK_STACK_SIZE 4096

// chyba zrobic to globalbnie i atomic setem zmieniac
enum class stand_state_t : std::uint8_t
{
    WAIT_FOR_CALIB = 0, // czekamy na calib z yolo
    IDLE,               // spimy poloczeni
    MOVING,             // rucb
    LOST_VISSION,       // yolo stracilo wizje
    ERROR
};

class TRACKER
{
    const char *TRACKER_TAG = "Tracker";
    SharedNavigationData &nav_data;
    stand_state_t stand_state{stand_state_t::WAIT_FOR_CALIB};
    volatile float curr_angle{};
    float latitude{};
    float longitude{};
    bool is_homed{false};

    float azimuth{}; // azymuth z yolo

    STEPPER_MOTOR Motor;
    TaskHandle_t MOTOR_TASK_HANDLE = NULL;

    static void motor_thread_trampoline(void *arg);

    void stepper_motor_thread_loop();

    void home_initialization();

    bool is_connection_timmeout();

    bool deadzone();
    
    float gps_angle();

public:
    TRACKER(SharedNavigationData &shared_data) : nav_data(shared_data) {}

    void start_motor_rtos_thread();
};