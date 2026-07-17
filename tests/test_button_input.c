#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "platform/button_input.h"

typedef struct {
    int32_t level;
    unsigned sequence;
    unsigned read_order;
    unsigned sink_order;
    bool pressed;
    uint32_t now_ms;
} Fixture;

static int32_t read_level(void *ctx)
{
    Fixture *fixture = ctx;
    fixture->read_order = ++fixture->sequence;
    return fixture->level;
}

static void capture(bool pressed, uint32_t now_ms, void *ctx)
{
    Fixture *fixture = ctx;
    fixture->sink_order = ++fixture->sequence;
    fixture->pressed = pressed;
    fixture->now_ms = now_ms;
}

void test_button_input(void)
{
    Fixture fixture = {.level = 0};
    button_input_poll(42u, read_level, capture, &fixture);
    assert(fixture.read_order == 1u && fixture.sink_order == 2u);
    assert(fixture.pressed && fixture.now_ms == 42u);

    fixture = (Fixture){.level = 1};
    button_input_poll(UINT32_MAX, read_level, capture, &fixture);
    assert(!fixture.pressed && fixture.now_ms == UINT32_MAX);

    fixture = (Fixture){.level = -1};
    button_input_poll(7u, read_level, capture, &fixture);
    assert(!fixture.pressed);
}
