#include "tracker_controller.hpp"
#include "shared_data.hpp"
#include "esp_timer.h"
#include <math.h>
#include "esp_log.h"

void TRACKER::home_initialization()
{
}

void TRACKER::motor_thread_trampoline(void *arg)
{
  TRACKER *motor_thread = static_cast<TRACKER *>(arg);

  motor_thread->stepper_motor_thread_loop();

  return;
}

bool TRACKER::deadzone_x()
{
  if (std::abs(nav_data.latest_yolo_error_x) > DEADZONE_X)
  {
    return true;
  }
  return false;
}
bool TRACKER::deadzone_y()
{
  if(std::abs(nav_data.latest_yolo_error_y) > DEADZONE_Y)
  {
    return true;
  }
  return false;
}

bool TRACKER::is_connection_timeout(uint32_t data_last_timestamp)
{
  bool err{false};

  std::uint32_t curr_time = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
  if (curr_time - data_last_timestamp > 500ULL)
  {
    // ESP_LOGI(TRACKER_TAG, "Wyjebalo sie komunikacja");
    err = true;
  }
  // pozniej dodamy naprzyklad timeout z gps esp nowczy tam lora

  return err;
}

float TRACKER::gps_angle()
{
  float d_lat = nav_data.target_gps_lat - nav_data.base_lat;
  float d_lon = (nav_data.target_gps_lon - nav_data.base_lon) * cosf(nav_data.base_lat * M_PI / 180.0f);

  return atan2f(d_lon, d_lat) * 180.0f / M_PI;
}

void TRACKER::start_motor_rtos_thread()
{
  xTaskCreatePinnedToCore(motor_thread_trampoline, "TASK MOTUR", MOTOR_TASK_STACK_SIZE, this, 5, &MOTOR_TASK_HANDLE, 0);
}

void TRACKER::stepper_motor_thread_loop()
{

  float human_angle = 0.0f;
  float step_deg = 1.5f;
  stand_state_t prev_state = stand_state;

  int servo_count{};
  for (;;)
  {
    switch (stand_state)
    {
    case stand_state_t::WAIT_FOR_CALIB:
      if (nav_data.request_homing.exchange(false, std::memory_order_relaxed))
      {
        nav_data.yolo_last_timestamp = esp_timer_get_time() / 1000ULL;
        axis_x.set_current_pos_as_azimuth(nav_data.base_azimuth);
        stand_state = stand_state_t::IDLE;
        is_homed = true;
        ESP_LOGI(TRACKER_TAG, "WAIT FOR CALIB STATE");
      }
      break;

    case stand_state_t::MOVING:

      if (axis_x.is_stopped())
      {
        stand_state = stand_state_t::IDLE;
      }

    case stand_state_t::IDLE:
      if (!is_connection_timeout(nav_data.yolo_last_timestamp))
      {
        if (nav_data.yolo_target_lost)
        {
          stand_state = stand_state_t::LOST_VISSION;
          break;
        }

     
          if (nav_data.new_yolo_data.exchange(false))
          {
            if(deadzone_x()) axis_x.move_by_delta(nav_data.latest_yolo_error_x);
            if(deadzone_y()) axis_y.move_by_delta(nav_data.latest_yolo_error_y);

            stand_state = stand_state_t::MOVING;

            axis_y.move_by_delta(nav_data.latest_yolo_error_y);
          }
        
      }
      else
      {
        stand_state = stand_state_t::ERROR;
      }
      break;

    case stand_state_t::ERROR:
      if (!is_connection_timeout(nav_data.yolo_last_timestamp) && is_homed &&is_connection_timeout(nav_data.gps_last_timestamp))
      {
        stand_state = stand_state_t::IDLE;
      }
      break;

    case stand_state_t::LOST_VISSION:
      if (!is_connection_timeout(nav_data.yolo_last_timestamp) && nav_data.yolo_target_lost == false)
      {
        stand_state = stand_state_t::IDLE;
        break;
      }
      if (nav_data.gps_last_timestamp)
      {
        stand_state = stand_state_t::ERROR;
      }
      if (nav_data.target_gps_lat != 0.0f && nav_data.target_gps_lon != 0.0f)
      {
        axis_x.move_to_angle(gps_angle());
      }

      // if(gps_timeout to tez sie doda pozniej nalora)
      // liczenie rzeczy z gpska

      break;

    default:
      break;
    }

    vTaskDelay(pdMS_TO_TICKS(20));

  }
}