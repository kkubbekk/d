#include "tracker_controller.hpp"
#include "shared_data.hpp"
#include "esp_timer.h"

void TRACKER::home_initialization()
{
}

void TRACKER::motor_thread_trampoline(void *arg)
{
  TRACKER *motor_thread = static_cast<TRACKER *>(arg);

  motor_thread->stepper_motor_thread_loop();

  return;
}

bool TRACKER::deadzone()
{
  if (std::abs(nav_data.latest_yolo_error) > DEADZONE)
  {
    return true;
  }
  return false;
}

bool TRACKER::is_connection_timmeout()
{
  bool err{false};

  std::uint32_t curr_time = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
  if (curr_time - nav_data.yolo_last_timestamp > 500ULL)
  {
    ESP_LOGI(TRACKER_TAG, "Wyjebalo sie komunikacja");
    err = true;
  }
  // pozniej dodamy naprzyklad timeout z gps esp nowczy tam lora

  return err;
}

void TRACKER::start_motor_rtos_thread()
{
  xTaskCreatePinnedToCore(motor_thread_trampoline, "TASK MOTUR", MOTOR_TASK_STACK_SIZE, this, 5, &MOTOR_TASK_HANDLE, 0);
}

void TRACKER::stepper_motor_thread_loop()
{
  Motor.motor_init(16, 50, 70);

  float human_angle = 0.0f;
  float step_deg = 1.5f;

  for (;;)
  {
    switch (stand_state)
    {
    case stand_state_t::WAIT_FOR_CALIB:
      if (nav_data.request_homing.exchange(false, std::memory_order_relaxed))
      {
        Motor.set_current_pos_as_zero();
        stand_state = stand_state_t::IDLE;
        is_homed = true;
      
      }
      break;

    case stand_state_t::MOVING:
      if (Motor.is_stopped())
      {
        stand_state = stand_state_t::IDLE;
   
      }

    case stand_state_t::IDLE:

      if (!is_connection_timmeout())
      {
        if (nav_data.yolo_target_lost)
        {
          stand_state = stand_state_t::LOST_VISSION;
          break;
        }

        if (deadzone())
        {
          if (nav_data.new_yolo_data.exchange(false))
          {
            Motor.move_by_delta(nav_data.latest_yolo_error);
            stand_state = stand_state_t::MOVING;
          }
        }
      }
      else
      {
        stand_state = stand_state_t::ERROR;
      }
      break;

      case stand_state_t::ERROR:
      if (!is_connection_timmeout() && is_homed)
      {
        stand_state = stand_state_t::IDLE;
      }
      break;

    case stand_state_t::LOST_VISSION:
      if (!is_connection_timmeout() && nav_data.yolo_target_lost == false)
      {
        stand_state = stand_state_t::IDLE;
        break;
      }
      // if(gps_timeout to tez sie doda pozniej nalora)
      // liczenie rzeczy z gpska

      break;

    default:
      break;
    }

    vTaskDelay(pdMS_TO_TICKS(20));

    // human_angle += step_deg;
    // if (human_angle >= 360.0f) {
    //     human_angle = 360.0f;
    //     step_deg = -1.5f; // Zawrot w lewo!
    // } else if (human_angle <= 0.0f) {
    //     human_angle = 0.0f;
    //     step_deg = 1.5f;  // Zawrot w prawo!
    // }

    // Motor.move_to_angle(180);

    //  Motor.move_to_angle(180);
    //   Motor.move_to_angle(180);
    // vTaskDelay(pdMS_TO_TICKS(1000));
    // Motor.move_to_angle(360);
    //      Motor.move_to_angle(360);
    //           Motor.move_to_angle(360);
    // vTaskDelay(pdMS_TO_TICKS(1000));
  }
}