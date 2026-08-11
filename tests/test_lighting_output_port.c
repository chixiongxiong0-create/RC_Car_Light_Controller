#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

/* The helper does not yet exist: this test defines the port's desired
 * threshold behavior before its hardware implementation is added. */
bool lighting_output_port_duty_is_on(uint16_t duty);

void test_lighting_output_port(void)
{
    assert(!lighting_output_port_duty_is_on(0u));
    assert(!lighting_output_port_duty_is_on(499u));
    assert(lighting_output_port_duty_is_on(500u));
    assert(lighting_output_port_duty_is_on(1000u));
    assert(lighting_output_port_duty_is_on(UINT16_MAX));
}
