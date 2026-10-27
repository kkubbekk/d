#include <cstdint>

#define CRC 0x86 //10000110

union TMC2209_DataFrame64
{
    struct __attribute__((packed)) 
    {
        std::uint8_t sync_res;
        std::uint8_t nodeaddr;
        std::uint8_t reg_addr;
        std::uint32_t data;
        std::uint8_t crc;
    } ;

    std::uint8_t bytes[8];
};

union RxRequest 
{
    struct __attribute__((packed)) 
    {
        std::uint8_t sync_res;
        std::uint8_t nodeaddr;
        std::uint8_t reg_addr;
        std::uint8_t crc;
    };

    std::uint8_t bytes[4];
};


inline uint8_t calculate_crc(uint8_t* frame,size_t length)
{
    // uint8_t* crc_rx = frame + (length -1);
    uint8_t calculated{};
    uint8_t current_byte;

    for(int i = 0;i<(length-1);i++)
    {
        current_byte = frame[i];
        for(int j =0;j<8;j++)
        {
            if((calculated>>7) ^ (current_byte&0x01))
            {
                calculated = (calculated<<1) ^ 0x07;
            }
            else
            {
                calculated= (calculated << 1);
            }
            current_byte = (current_byte >> 1);
        }
    }

    return calculated;

}