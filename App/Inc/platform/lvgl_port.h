#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#include <stdbool.h>
#include <stdint.h>

#define UI_WIDTH 320u
#define UI_HEIGHT 240u
#define PHYSICAL_WIDTH 240u
#define PHYSICAL_HEIGHT 320u
#define DRAW_LINES 24u

bool lvgl_port_init(void);
void lvgl_port_tick(uint32_t now_ms);
uint16_t lvgl_port_fps(void);

#endif
