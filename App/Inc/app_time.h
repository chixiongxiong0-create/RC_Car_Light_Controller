#pragma once

#include <stdbool.h>
#include <stdint.h>

uint32_t elapsed_ms(uint32_t now, uint32_t then);
bool time_reached(uint32_t now, uint32_t deadline);
