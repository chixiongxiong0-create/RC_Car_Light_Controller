#include <assert.h>

#include "app_time.h"

void test_app_time(void)
{
    assert(elapsed_ms(25u, 10u) == 15u);
    assert(elapsed_ms(5u, 0xfffffff0u) == 21u);
    assert(time_reached(100u, 100u));
    assert(!time_reached(99u, 100u));
    assert(time_reached(5u, 0xfffffff0u));
}
