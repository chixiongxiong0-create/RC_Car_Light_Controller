#include "platform/button_input.h"

#include <stddef.h>

void button_input_poll(uint32_t now_ms, ButtonInputRead read,
                       ButtonInputSink sink, void *ctx)
{
    if (read == NULL || sink == NULL) {
        return;
    }
    /* PF1 is externally pulled high and the user switch closes it to ground. */
    const bool pressed = read(ctx) == 0;
    sink(pressed, now_ms, ctx);
}
