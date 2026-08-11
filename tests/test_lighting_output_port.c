#include <assert.h>
#include <stdint.h>

#include "platform/lighting_output_port.h"

void test_lighting_output_port(void)
{
    assert(!lighting_output_port_duty_is_on(0u));
    assert(!lighting_output_port_duty_is_on(499u));
    assert(lighting_output_port_duty_is_on(500u));
    assert(lighting_output_port_duty_is_on(1000u));
    assert(lighting_output_port_duty_is_on(UINT16_MAX));
}
