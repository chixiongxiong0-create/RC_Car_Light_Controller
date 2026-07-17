#include "app_time.h"

uint32_t elapsed_ms(uint32_t now, uint32_t then)
{
    return now - then;
}

bool time_reached(uint32_t now, uint32_t deadline)
{
    return (int32_t)(now - deadline) >= 0;
}
