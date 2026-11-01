
#pragma once

#include "esp_wifi.h"
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <cstring>
#include "esp_now.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "packet.hpp"
#include "freertos/semphr.h"



class ESP_NOW
{
    uint8_t peer_addr_[6];
    inline static SemaphoreHandle_t  semhandle = NULL;
    inline static QueueHandle_t QueueFromEsp;

    public:
        
        ESP_NOW(uint8_t* peer_addr)
        {
            memcpy(peer_addr_,peer_addr,(sizeof(peer_addr_)));
        }   
        
        esp_err_t ESPNOW_LINK();
     
       
        

        static void espnow_send_cb(const wifi_tx_info_t *tx_info, esp_now_send_status_t status);
      
          
        static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);

        bool espnow_receive_data(packet_t * buff,uint8_t ms_timeout);

        void espnow_send_data(packet_t *buff);
       
     
        
};