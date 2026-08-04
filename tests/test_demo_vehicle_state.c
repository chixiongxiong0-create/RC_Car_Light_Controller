#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "demo_vehicle_state.h"

static bool any_float_changed(const VehicleState samples[], size_t count,
                              size_t offset)
{
    const float first = *(const float *)((const char *)&samples[0] + offset);
    for (size_t i = 1u; i < count; ++i) {
        if (*(const float *)((const char *)&samples[i] + offset) != first) {
            return true;
        }
    }
    return false;
}

static bool any_u16_changed(const VehicleState samples[], size_t count,
                            size_t offset)
{
    const uint16_t first = *(const uint16_t *)((const char *)&samples[0] + offset);
    for (size_t i = 1u; i < count; ++i) {
        if (*(const uint16_t *)((const char *)&samples[i] + offset) != first) {
            return true;
        }
    }
    return false;
}

static bool any_u8_changed(const VehicleState samples[], size_t count,
                           size_t offset)
{
    const uint8_t first = *(const uint8_t *)((const char *)&samples[0] + offset);
    for (size_t i = 1u; i < count; ++i) {
        if (*(const uint8_t *)((const char *)&samples[i] + offset) != first) {
            return true;
        }
    }
    return false;
}

int main(void)
{
    const uint32_t timestamps[] = {0u, 1000u, 5000u, 12000u};
    VehicleState samples[sizeof timestamps / sizeof timestamps[0]];
    VehicleState repeated_a;
    VehicleState repeated_b;

    for (size_t i = 0u; i < sizeof samples / sizeof samples[0]; ++i) {
        demo_vehicle_state_sample(timestamps[i], 4u, &samples[i]);
        assert(samples[i].throttle >= -1.0f && samples[i].throttle <= 1.0f);
        assert(samples[i].steering >= -1.0f && samples[i].steering <= 1.0f);
        assert(samples[i].heading_deg >= 0.0f && samples[i].heading_deg < 360.0f);
        assert(samples[i].roll_deg >= -20.0f && samples[i].roll_deg <= 20.0f);
        assert(samples[i].pitch_deg >= -20.0f && samples[i].pitch_deg <= 20.0f);
        assert(samples[i].aux_page == 0.0f);
        assert(!samples[i].armed);
        assert(samples[i].link == LINK_STARTING);
        assert(samples[i].battery_v > 14.8f);
        assert(!samples[i].battery_valid);
        assert(samples[i].last_msp_ms == 0u);
        assert(samples[i].last_rc_ms == 0u);
        assert(samples[i].last_attitude_ms == 0u);
    }

    assert(any_float_changed(samples, 4u, offsetof(VehicleState, throttle)));
    assert(any_float_changed(samples, 4u, offsetof(VehicleState, steering)));
    assert(any_float_changed(samples, 4u, offsetof(VehicleState, roll_deg)));
    assert(any_float_changed(samples, 4u, offsetof(VehicleState, pitch_deg)));
    assert(any_float_changed(samples, 4u, offsetof(VehicleState, heading_deg)));
    assert(any_u16_changed(samples, 4u, offsetof(VehicleState, rssi)));
    assert(any_u8_changed(samples, 4u, offsetof(VehicleState, gps_sats)));

    for (uint8_t cells = 2u; cells <= 6u; ++cells) {
        VehicleState cell_sample;
        demo_vehicle_state_sample(2500u, cells, &cell_sample);
        assert(cell_sample.battery_v > 3.7f * (float)cells);
        assert(!cell_sample.battery_valid);
    }

    VehicleState unknown_cells;
    demo_vehicle_state_sample(2500u, 0u, &unknown_cells);
    assert(unknown_cells.battery_v > 0.0f);
    assert(!unknown_cells.battery_valid);

    demo_vehicle_state_sample(5000u, 4u, &repeated_a);
    demo_vehicle_state_sample(5000u, 4u, &repeated_b);
    assert(memcmp(&repeated_a, &repeated_b, sizeof repeated_a) == 0);
    return 0;
}
