#pragma once

#include <stdbool.h>
#include <stdint.h>

bool f469_display_touch_init(void);
bool f469_display_set_brightness(uint8_t value);
void f469_display_touch_tick(uint32_t now_ms);
bool f469_touch_available(void);
void f469_touch_health_check(void);
