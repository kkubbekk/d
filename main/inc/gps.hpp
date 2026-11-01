#pragma once

struct __attribute__((packed)) gps_struct_t {
     float latitude;
    float longitude;
    float azimuth;
    bool homing;
};