#pragma once
#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(MOTOR_EVENT_BASE);

enum motor_event_id_t {
    YOLO_CMND_EVENT = 0,
    GPS_CMND_EVENT
};