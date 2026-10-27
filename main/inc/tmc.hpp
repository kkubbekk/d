#include <cstdint>
#include <driver/uart.h>
#include "tmc_reg.hpp"
#include "uart_frames.hpp"
#define BUFF_SIZE 64*3

#define UART_TX GPIO_NUM_17
#define UART_RX GPIO_NUM_16
#define STALL_GUARD_INT_PIN GPIO_NUM_22 
#define NODEADDR 0 // zalezy od tego jakie zasilanie podamy na ms1 msi2
#define TMC_SYNC_BIT 0x05

 //ms1 msi2 zwarte do masy wiec microstep 1/8


class TMC
{   
    //NARAZE TUTAJ WARTOSCI Z CIPY!! POZNIEJ SIE ZROBI SHADOWING I TASKA DO WYSYLU;
    std::uint32_t chopconf_state = 0x10000053;

   

    std::uint32_t gconf_state = 
      (1 << 8)   
    | (1 << 7)
    | (1 << 6)
    | (0 << 3)
    | (0 << 2)     
    | (1 << 0);    //

    const char *TMC_TAG = "TMC";



      void tmc2209_write_cmnd(std::uint8_t reg,std::uint32_t data)
    {
        TMC2209_DataFrame64 frame;
        
       
        frame.nodeaddr = NODEADDR;
        frame.sync_res = TMC_SYNC_BIT;
        frame.data = __builtin_bswap32(data);
        frame.reg_addr = reg | 0x80;
        frame.crc = calculate_crc(frame.bytes,sizeof(frame));
        
        uart_write_bytes(UART_NUM_1,&frame,sizeof(frame));

        uart_wait_tx_done(UART_NUM_1, pdMS_TO_TICKS(200));  
        
        //fluishujemy na
         std::uint8_t dummy_read[4];
         uart_read_bytes(UART_NUM_1,dummy_read,sizeof(dummy_read),pdMS_TO_TICKS(50));
    }
    //trza sie zastanowic czy zwaracmy tylko data czy calego strukta
    std::uint32_t tmc2209_read_data(std::uint8_t reg)
    {
        RxRequest rx_frame;
        rx_frame.nodeaddr = NODEADDR;
        rx_frame.reg_addr = reg;
        rx_frame.sync_res = TMC_SYNC_BIT;
        rx_frame.crc = calculate_crc(rx_frame.bytes,sizeof(rx_frame));

       
        uart_flush(UART_NUM_1);
        uart_write_bytes(UART_NUM_1,rx_frame.bytes,sizeof(rx_frame));
        uart_wait_tx_done(UART_NUM_1, pdMS_TO_TICKS(300)); // 

        std::uint8_t dummy_buffer[4];

        std::uint8_t echo = uart_read_bytes(UART_NUM_1,dummy_buffer,sizeof(rx_frame),pdMS_TO_TICKS(50)) ;

        if(echo != 4)
        {
            ESP_LOGE(TMC_TAG,"ODEBRANO TYLKO %d/4 bajtow",echo);
        }
      
        

        TMC2209_DataFrame64 rx_frame_response;

        echo = uart_read_bytes(UART_NUM_1,&rx_frame_response,sizeof(rx_frame_response),pdMS_TO_TICKS(50));

        if(echo != 8)
        {
            ESP_LOGE(TMC_TAG, "Błąd echa! Odebrano tylko %d/4 bajtów..", echo);
        }

    if(calculate_crc(rx_frame_response.bytes,sizeof(rx_frame_response)) != rx_frame_response.crc)
        {
            ESP_LOGE(TMC_TAG,"COS SIE ZJEBALO");
            return 0;
        }
        return __builtin_bswap32(rx_frame_response.data);


    }

     static void IRAM_ATTR  stall_guard_isr_trampoline(void *arg)
    {
        TMC* tmc = static_cast<TMC*>(arg);

        tmc->stallguard_intr_isr_handler(arg);
    } 

     void IRAM_ATTR  stallguard_intr_isr_handler(void * arg)
    {
       isr_test =1;
    }
      public:

    volatile std::uint8_t isr_test;

    void set_current(std::uint8_t irun_percentage,std::uint8_t ihold_percentage)
    {
        std::uint8_t ihold = (ihold_percentage*31)/(100);

        std::uint8_t irun = (irun_percentage*31    )/(100);

        std::uint32_t payload{};

        uint32_t iholddelay_val = 4;

        payload = (iholddelay_val <<16) | (irun << 8) | (ihold <<0);

        tmc2209_write_cmnd(IHOLD_IRUN,payload);


    }
    std::uint16_t read_current_val()
    {
       return tmc2209_read_data(IHOLD_IRUN);
    }


    void tmc_init()
    {
        esp_err_t ret = ESP_FAIL;       // uart w half duplex do komunikacji z tmc2209
        uart_config_t uart_conf =
        {
                .baud_rate = 115200,
                .data_bits = UART_DATA_8_BITS,
                .parity    = UART_PARITY_DISABLE,
                .stop_bits = UART_STOP_BITS_1,
                .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        };
        ret = uart_param_config(UART_NUM_1, &uart_conf);
        ret = uart_set_pin(UART_NUM_1,UART_TX,UART_RX,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE);
        
        ret = uart_driver_install(UART_NUM_1, BUFF_SIZE, BUFF_SIZE, 0, NULL, 0);

        tmc2209_write_cmnd(GCONF,gconf_state);

        // gpio na intterupt stallguard
                    const gpio_config_t int_gpio =
                    {
                        .pin_bit_mask = (1ULL << STALL_GUARD_INT_PIN),
                        .mode = GPIO_MODE_INPUT,
                        .pull_up_en = GPIO_PULLUP_DISABLE,
                        .pull_down_en = GPIO_PULLDOWN_ENABLE,
                        .intr_type = GPIO_INTR_POSEDGE,
                    };
                    
                    ret = gpio_config(&int_gpio);

                    ret = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);


                   
                 ret = gpio_isr_handler_add(STALL_GUARD_INT_PIN,stall_guard_isr_trampoline,this);
    
         }
    std::uint16_t set_microsteps(std::uint16_t motor_microsteps)
    
    {
       std::uint8_t mres;

       switch (motor_microsteps)
       {
        case 1:
         mres = 8;
         break;
        case 2:
            mres = 7;
            break;
        case 4:
            mres = 6;
            break;
        case 8:
            mres = 5;
            break;
        case 16:
            mres =4;
            break;
        case 32:
            mres =3;
            break;
        case 64:
            mres =2;
            break;
        case 128:
            mres = 1;
            break;
        case 256:
            mres =0;
            break;
        default:
            ESP_LOGE(TMC_TAG,"ZLA LICZBA microstepwo");
            return 0;

       }
       
       
       chopconf_state &=  ~ ( 0xF <<24);
       chopconf_state |= (mres <<24);

       tmc2209_write_cmnd(CHOPCONF,chopconf_state);

       return motor_microsteps;
    }

    //imo lepiej korzystac z gpio tylko motor dir i chuj
    void change_direction()
    {
        gconf_state ^= (1<<3);

        tmc2209_write_cmnd(GCONF,gconf_state);
    }
    
      void set_stallguard_activation_val(uint8_t val)
      {
        tmc2209_write_cmnd(SGTHRS,(uint32_t)val);

      }
    
      void set_stall_guard_vel_starting_point()
      {
        tmc2209_write_cmnd(TCOOLTHRS,0xFFFFF);
      }

      std::uint16_t read_sg_result_val()
      {
        
        return (uint16_t) (tmc2209_read_data(SG_RESULT) & 0x000003FF);
      }
      

    //   std::uint16_t read_stallguard_status()
    //   {
        
    //   }

      

};