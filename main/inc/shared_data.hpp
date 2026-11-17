#pragma once
#include <atomic>
#include <cstdint>

struct SharedNavigationData {
    // 1. DANE STARTOWE
    std::atomic<float> base_azimuth{0.0f};     
    std::atomic<float> base_lat{0.0f};        
    std::atomic<float> base_lon{0.0f};         
    std::atomic<bool>  request_homing{false};  

    // 2. DANE BIEŻĄCE 
    std::atomic<float> latest_yolo_error{0.0f};
    std::atomic<bool>  new_yolo_data{false};
    std::atomic<float> target_gps_lat{0.0f};
    std::atomic<float> target_gps_lon{0.0f};
    std::atomic<bool>  yolo_target_lost{true};
    std::atomic<std::uint32_t> yolo_last_timestamp{0};
    std::atomic<std::uint32_t> gps_last_timestamp{0};
     

    
    std::atomic<bool> is_homed{false};          
};