//do przemyslenia na pewno timestampy
//i strukturka
#pragma once

typedef struct
{
    uint8_t data[20];
    int data_len;
} packet_t;

static_assert(sizeof(packet_t) < 250, "EXCEEDED ESP_NOW v1 MAX_DATA_LENGTH");