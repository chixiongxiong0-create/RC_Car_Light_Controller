#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#include <stdint.h>

#define UI_WIDTH 320u
#define UI_HEIGHT 240u
#define PHYSICAL_WIDTH 240u
#define PHYSICAL_HEIGHT 320u
#define DRAW_LINES 24u

void lvgl_port_init(void);
void lvgl_port_tick(uint32_t now_ms);

#endif
