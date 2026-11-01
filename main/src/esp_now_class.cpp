#include "esp_now_class.hpp"

static const char* TAG = "espnow";

esp_err_t ESP_NOW::ESPNOW_LINK()
        {
            //Wifi initialization required for ESP_NOW
           esp_err_t err;

    if ((err = nvs_flash_init()) != ESP_OK) return err;
    if ((err = esp_netif_init()) != ESP_OK) return err;
    if ((err = esp_event_loop_create_default()) != ESP_OK) return err;
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if ((err = esp_wifi_init(&cfg)) != ESP_OK) return err;
    if ((err = esp_wifi_set_storage(WIFI_STORAGE_RAM)) != ESP_OK) return err;
    if ((err = esp_wifi_set_mode(WIFI_MODE_STA)) != ESP_OK) return err;
    if ((err = esp_wifi_start()) != ESP_OK) return err;
    if ((err = esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE)) != ESP_OK) return err;

    if ((err = esp_now_init()) != ESP_OK) return err;

    //queues definition to handle data from callbacks (packet.hpp)
    QueueFromEsp = xQueueCreate(10, sizeof(packet_t));
    

    //semdefinition to make sure we can transmit data without race condition;
    semhandle = xSemaphoreCreateBinary();
    xSemaphoreGive(semhandle);

    if(QueueFromEsp == NULL || semhandle == NULL)
    {
        return ESP_ERR_NO_MEM;
    }



    // callback register
    if ((err = esp_now_register_recv_cb(espnow_recv_cb)) != ESP_OK) return err;
    if ((err = esp_now_register_send_cb(espnow_send_cb)) != ESP_OK) return err;

    // adding peer info
    esp_now_peer_info_t peer = {};            
    memcpy(peer.peer_addr, peer_addr_, 6);
    peer.ifidx = WIFI_IF_STA;
    peer.channel = 1;
    peer.encrypt = false;
    
    if ((err = esp_now_add_peer(&peer)) != ESP_OK) return err;

    //wszystko g
    return ESP_OK;

        

           
    }
        bool ESP_NOW::espnow_receive_data(packet_t * buff,uint8_t ms_timeout)
         {
            //odbieramy dane z kolejki
            if(xQueueReceive(QueueFromEsp,buff,pdMS_TO_TICKS(ms_timeout)) != pdTRUE)
            {
                ESP_LOGE(TAG, "NIEODEBRANODANYCH Z KOLEJKI");
                return false;
            } 
            return true;
            
         }

        void ESP_NOW::espnow_send_data(packet_t *buff)
        {
            if (xSemaphoreTake(semhandle, pdMS_TO_TICKS(100)) != pdTRUE) {
                 ESP_LOGW(TAG, "Semaphore timeout");
                 return;
                 }

            esp_err_t err = esp_now_send(peer_addr_, reinterpret_cast<const uint8_t *>(buff),sizeof(*buff));

             if (err != ESP_OK) 
             {
                ESP_LOGE(TAG, "esp_now_send: %s", esp_err_to_name(err));
                xSemaphoreGive(semhandle);
             }
        }

         //callback for band 
        void ESP_NOW::espnow_send_cb(uint8_t peer_adrr,const wifi_tx_info_t *tx_info, esp_now_send_status_t status)
         {
            if (semhandle != NULL) 
            {
                xSemaphoreGive(semhandle);
                
            }
        }       
          
         void ESP_NOW::espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
        {
            packet_t buff = {};

        }




