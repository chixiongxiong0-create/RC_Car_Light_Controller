#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef int32_t (*ButtonInputRead)(void *ctx);
typedef void (*ButtonInputSink)(bool pressed, uint32_t now_ms, void *ctx);

void button_input_poll(uint32_t now_ms, ButtonInputRead read,
                       ButtonInputSink sink, void *ctx);
