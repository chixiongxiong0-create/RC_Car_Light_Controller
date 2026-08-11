#include <assert.h>
#include <math.h>
#include <string.h>

#include "vehicle_state.h"

enum {
    MSP_STATUS = 101,
    MSP_RAW_GPS = 106,
    MSP_ATTITUDE = 108,
    MSP_ANALOG = 110,
    MSP_RC = 105
};

static void near(float actual, float expected)
{
    assert(fabsf(actual - expected) < 0.001f);
}

static MspFrame frame(uint16_t command, const uint8_t *payload, uint8_t length)
{
    MspFrame result = {.command = command, .length = length};
    memcpy(result.payload, payload, length);
    return result;
}

static void test_lighting_rc_validity_initializes_safe(void)
{
    vehicle_state_init();
    const VehicleState *state = vehicle_state_get();
    assert(!state->lighting_rc_valid);
    assert(state->last_lighting_rc_ms == 0u);
    near(state->aux6, -1.0f);
    near(state->aux7, -1.0f);
    near(state->aux8, -1.0f);
    near(state->aux9, -1.0f);
}

static void test_decodes_supported_frames(void)
{
    const uint8_t rc_payload[] = {
        0xe8, 0x03, 0xdc, 0x05, 0xd0, 0x07, 0xdc, 0x05,
        0xd0, 0x07, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05
    };
    const uint8_t attitude_payload[] = {0x7b, 0x00, 0x38, 0xff, 0x0e, 0x01};
    const uint8_t analog_payload[] = {123, 0x34, 0x12, 0x4d, 0x03, 0x78, 0x56};
    const uint8_t gps_payload[] = {
        2, 11, 1, 2, 3, 4, 5, 6, 7, 8, 0x34, 0x12, 0x78, 0x56,
        0xbc, 0x9a, 0xf0, 0xde
    };
    const uint8_t status_payload[] = {
        1, 2, 3, 4, 5, 6, 1, 1, 0, 0, 7
    };

    vehicle_state_init();
    MspFrame input = frame(MSP_RC, rc_payload, sizeof rc_payload);
    assert(vehicle_state_on_msp(&input, 100u));
    const VehicleState *state = vehicle_state_get();
    near(state->steering, -1.0f);
    near(state->throttle, 1.0f);
    near(state->aux_page, 1.0f);
    near(state->aux6, -1.0f);
    near(state->aux7, -1.0f);
    near(state->aux8, -1.0f);
    near(state->aux9, -1.0f);
    assert(state->last_rc_ms == 100u && state->last_msp_ms == 100u);
    assert(state->link == LINK_OK);
    assert(!state->battery_valid);

    input = frame(MSP_ATTITUDE, attitude_payload, sizeof attitude_payload);
    assert(vehicle_state_on_msp(&input, 110u));
    state = vehicle_state_get();
    near(state->roll_deg, 12.3f);
    near(state->pitch_deg, -20.0f);
    near(state->heading_deg, 270.0f);
    assert(state->last_attitude_ms == 110u);

    input = frame(MSP_ANALOG, analog_payload, sizeof analog_payload);
    assert(vehicle_state_on_msp(&input, 120u));
    state = vehicle_state_get();
    near(state->battery_v, 12.3f);
    assert(state->rssi == 845u);
    assert(state->battery_valid);

    input = frame(MSP_RAW_GPS, gps_payload, sizeof gps_payload);
    assert(vehicle_state_on_msp(&input, 130u));
    assert(vehicle_state_get()->gps_sats == 11u);

    input = frame(MSP_STATUS, status_payload, sizeof status_payload);
    assert(vehicle_state_on_msp(&input, 140u));
    assert(vehicle_state_get()->armed);
    assert(vehicle_state_get()->mode_flags == 0x00000101u);
}

static void test_decodes_lighting_aux_channels(void)
{
    const uint8_t rc_payload[] = {
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05,
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05,
        0xdc, 0x05, 0xe8, 0x03, 0xe2, 0x04, 0xd6, 0x06,
        0xd0, 0x07
    };

    vehicle_state_init();
    MspFrame input = frame(MSP_RC, rc_payload, sizeof rc_payload);
    assert(vehicle_state_on_msp(&input, 1u));
    const VehicleState *state = vehicle_state_get();
    near(state->aux6, -1.0f);
    near(state->aux7, -0.5f);
    near(state->aux8, 0.5f);
    near(state->aux9, 1.0f);
    assert(state->lighting_rc_valid);
    assert(state->last_lighting_rc_ms == 1u);
}

static void test_clamps_lighting_aux_channels(void)
{
    const uint8_t rc_payload[] = {
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05,
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05,
        0xdc, 0x05, 0x00, 0x00, 0x2c, 0x01, 0xb8, 0x0b,
        0xff, 0xff
    };

    vehicle_state_init();
    MspFrame input = frame(MSP_RC, rc_payload, sizeof rc_payload);
    assert(vehicle_state_on_msp(&input, 1u));
    const VehicleState *state = vehicle_state_get();
    near(state->aux6, -1.0f);
    near(state->aux7, -1.0f);
    near(state->aux8, 1.0f);
    near(state->aux9, 1.0f);
}

static void test_short_rc_frame_resets_lighting_aux_channels_to_safe_value(void)
{
    const uint8_t full_rc_payload[] = {
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05,
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05,
        0xdc, 0x05, 0xd0, 0x07, 0xd0, 0x07, 0xd0, 0x07,
        0xd0, 0x07
    };
    const uint8_t short_rc_payload[] = {
        0xe8, 0x03, 0xdc, 0x05, 0xd0, 0x07, 0xdc, 0x05,
        0xd0, 0x07
    };

    vehicle_state_init();
    MspFrame input = frame(MSP_RC, full_rc_payload, sizeof full_rc_payload);
    assert(vehicle_state_on_msp(&input, 1u));
    near(vehicle_state_get()->aux6, 1.0f);
    assert(vehicle_state_get()->lighting_rc_valid);
    assert(vehicle_state_get()->last_lighting_rc_ms == 1u);

    input = frame(MSP_RC, short_rc_payload, sizeof short_rc_payload);
    assert(vehicle_state_on_msp(&input, 2u));
    const VehicleState *state = vehicle_state_get();
    near(state->steering, -0.35f);
    near(state->throttle, 0.35f);
    near(state->aux_page, 0.35f);
    near(state->aux6, -1.0f);
    near(state->aux7, -1.0f);
    near(state->aux8, -1.0f);
    near(state->aux9, -1.0f);
    assert(!state->lighting_rc_valid);
    assert(state->last_lighting_rc_ms == 1u);
    assert(state->last_rc_ms == 2u);
}

static void test_short_rc_frame_clears_lighting_aux_while_lost(void)
{
    const uint8_t full_rc_payload[] = {
        0xe8, 0x03, 0xdc, 0x05, 0xd0, 0x07, 0xdc, 0x05,
        0xd0, 0x07, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05,
        0xdc, 0x05, 0xd0, 0x07, 0xd0, 0x07, 0xd0, 0x07,
        0xd0, 0x07
    };
    const uint8_t short_rc_payload[] = {
        0xd0, 0x07, 0xdc, 0x05, 0xe8, 0x03, 0xdc, 0x05,
        0xe8, 0x03
    };

    vehicle_state_init();
    MspFrame input = frame(MSP_RC, full_rc_payload, sizeof full_rc_payload);
    assert(vehicle_state_on_msp(&input, 100u));
    vehicle_state_tick(2100u);
    assert(vehicle_state_get()->link == LINK_LOST);
    const VehicleState frozen = *vehicle_state_get();

    input = frame(MSP_RC, short_rc_payload, sizeof short_rc_payload);
    assert(vehicle_state_on_msp(&input, 2200u));
    const VehicleState *state = vehicle_state_get();
    assert(state->link == LINK_LOST);
    near(state->steering, frozen.steering);
    near(state->throttle, frozen.throttle);
    near(state->aux_page, frozen.aux_page);
    near(state->aux6, -1.0f);
    near(state->aux7, -1.0f);
    near(state->aux8, -1.0f);
    near(state->aux9, -1.0f);
}

static void test_lighting_rc_expires_independently_across_tick_wrap(void)
{
    const uint8_t full_rc_payload[] = {
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05,
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05,
        0xdc, 0x05, 0xd0, 0x07, 0xd0, 0x07, 0xd0, 0x07,
        0xd0, 0x07
    };
    const uint8_t analog_payload[] = {123, 0, 0, 0x4d, 0x03, 0, 0};
    const uint32_t full_rc_ms = UINT32_MAX - 200u;

    vehicle_state_init();
    MspFrame input = frame(MSP_RC, full_rc_payload, sizeof full_rc_payload);
    assert(vehicle_state_on_msp(&input, full_rc_ms));
    assert(vehicle_state_get()->lighting_rc_valid);
    near(vehicle_state_get()->aux6, 1.0f);

    input = frame(MSP_ANALOG, analog_payload, sizeof analog_payload);
    assert(vehicle_state_on_msp(&input, 100u));
    vehicle_state_tick(298u);
    assert(vehicle_state_get()->link == LINK_OK);
    assert(vehicle_state_get()->lighting_rc_valid);

    vehicle_state_tick(300u);
    const VehicleState *state = vehicle_state_get();
    assert(state->link == LINK_OK);
    assert(!state->lighting_rc_valid);
    assert(state->last_lighting_rc_ms == full_rc_ms);
    near(state->aux6, -1.0f);
    near(state->aux7, -1.0f);
    near(state->aux8, -1.0f);
    near(state->aux9, -1.0f);
}

static void test_channel_mapping_filter_and_deadband(void)
{
    const uint8_t center[] = {
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05,
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05
    };
    const uint8_t small_offset[] = {
        0xea, 0x05, 0xdc, 0x05, 0xce, 0x05, 0xdc, 0x05,
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05
    };
    const uint8_t extremes[] = {
        0xf4, 0x01, 0xdc, 0x05, 0xc4, 0x09, 0xdc, 0x05,
        0xc4, 0x09, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05
    };
    const uint8_t full_positive[] = {
        0xd0, 0x07, 0xdc, 0x05, 0xd0, 0x07, 0xdc, 0x05,
        0xd0, 0x07, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05
    };
    vehicle_state_init();
    MspFrame input = frame(MSP_RC, center, sizeof center);
    assert(vehicle_state_on_msp(&input, 1u));
    near(vehicle_state_get()->steering, 0.0f);
    near(vehicle_state_get()->throttle, 0.0f);
    input = frame(MSP_RC, small_offset, sizeof small_offset);
    assert(vehicle_state_on_msp(&input, 2u));
    near(vehicle_state_get()->steering, 0.0f);
    near(vehicle_state_get()->throttle, 0.0f);

    input = frame(MSP_RC, full_positive, sizeof full_positive);
    assert(vehicle_state_on_msp(&input, 3u));
    near(vehicle_state_get()->steering, 0.35f);
    near(vehicle_state_get()->throttle, 0.35f);
    near(vehicle_state_get()->aux_page, 0.35f);

    vehicle_state_init();
    input = frame(MSP_RC, extremes, sizeof extremes);
    assert(vehicle_state_on_msp(&input, 4u));
    near(vehicle_state_get()->steering, -1.0f);
    near(vehicle_state_get()->throttle, 1.0f);
    near(vehicle_state_get()->aux_page, 1.0f);

    const uint8_t level[] = {0, 0, 0, 0, 0, 0};
    const uint8_t tilted[] = {100, 0, 156, 255, 100, 0};
    input = frame(MSP_ATTITUDE, level, sizeof level);
    assert(vehicle_state_on_msp(&input, 5u));
    input = frame(MSP_ATTITUDE, tilted, sizeof tilted);
    assert(vehicle_state_on_msp(&input, 6u));
    near(vehicle_state_get()->roll_deg, 2.5f);
    near(vehicle_state_get()->pitch_deg, -2.5f);
    near(vehicle_state_get()->heading_deg, 25.0f);
}

static void test_malformed_is_atomic(void)
{
    const uint8_t valid[] = {100, 0, 0, 0, 0x22, 0x11, 0, 0};
    const uint8_t bad[] = {250, 0, 0, 0};
    const uint8_t status[] = {0, 0, 0, 0, 0, 0, 0x79, 0x56, 0x34, 0x12, 0};
    vehicle_state_init();
    MspFrame input = frame(MSP_ANALOG, valid, 7u);
    assert(vehicle_state_on_msp(&input, 10u));
    input = frame(MSP_STATUS, status, sizeof status);
    assert(vehicle_state_on_msp(&input, 11u));
    assert(vehicle_state_get()->mode_flags == 0x12345679u);
    const VehicleState before = *vehicle_state_get();
    input = frame(MSP_ANALOG, bad, sizeof bad);
    assert(!vehicle_state_on_msp(&input, 999u));
    assert(memcmp(&before, vehicle_state_get(), sizeof before) == 0);

    const struct {
        uint16_t command;
        uint8_t length;
    } malformed[] = {
        {MSP_RC, 9u},
        {MSP_ATTITUDE, 5u},
        {MSP_ANALOG, 6u},
        {MSP_RAW_GPS, 17u},
        {MSP_STATUS, 10u}
    };
    const uint8_t zeros[17] = {0};
    for (size_t i = 0; i < sizeof malformed / sizeof malformed[0]; ++i) {
        input = frame(malformed[i].command, zeros, malformed[i].length);
        assert(!vehicle_state_on_msp(&input, 1000u + (uint32_t)i));
        assert(memcmp(&before, vehicle_state_get(), sizeof before) == 0);
    }
}

static void test_malformed_gps_does_not_advance_recovery(void)
{
    const uint8_t rc[] = {
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05,
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05
    };
    const uint8_t analog[] = {120, 0, 0, 0x20, 0x03, 0, 0};
    const uint8_t malformed_gps[17] = {2, 15};
    vehicle_state_init();
    MspFrame input = frame(MSP_RC, rc, sizeof rc);
    assert(vehicle_state_on_msp(&input, 100u));
    vehicle_state_tick(2100u);
    assert(vehicle_state_get()->link == LINK_LOST);

    const VehicleState before = *vehicle_state_get();
    input = frame(MSP_RAW_GPS, malformed_gps, sizeof malformed_gps);
    assert(!vehicle_state_on_msp(&input, 2200u));
    assert(memcmp(&before, vehicle_state_get(), sizeof before) == 0);

    input = frame(MSP_ANALOG, analog, sizeof analog);
    assert(vehicle_state_on_msp(&input, 2450u));
    assert(vehicle_state_on_msp(&input, 2700u));
    assert(vehicle_state_on_msp(&input, 2949u));
    assert(vehicle_state_get()->link == LINK_LOST);
    assert(vehicle_state_on_msp(&input, 2950u));
    assert(vehicle_state_get()->link == LINK_OK);
}

static void test_all_fast_values_freeze_while_lost(void)
{
    const uint8_t live_rc[] = {
        0xe8, 0x03, 0xdc, 0x05, 0xd0, 0x07, 0xdc, 0x05,
        0xd0, 0x07, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05
    };
    const uint8_t changed_rc[] = {
        0xd0, 0x07, 0xdc, 0x05, 0xe8, 0x03, 0xdc, 0x05,
        0xe8, 0x03, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05
    };
    const uint8_t live_attitude[] = {100, 0, 56, 255, 90, 0};
    const uint8_t changed_attitude[] = {200, 0, 156, 255, 180, 0};
    const uint8_t analog[] = {126, 0, 0, 0x09, 0x03, 0, 0};
    const uint8_t gps[18] = {2, 12};

    vehicle_state_init();
    MspFrame input = frame(MSP_RC, live_rc, sizeof live_rc);
    assert(vehicle_state_on_msp(&input, 100u));
    input = frame(MSP_ATTITUDE, live_attitude, sizeof live_attitude);
    assert(vehicle_state_on_msp(&input, 110u));
    vehicle_state_tick(2110u);
    assert(vehicle_state_get()->link == LINK_LOST);
    const VehicleState frozen = *vehicle_state_get();

    input = frame(MSP_RC, changed_rc, sizeof changed_rc);
    assert(vehicle_state_on_msp(&input, 2200u));
    input = frame(MSP_ATTITUDE, changed_attitude, sizeof changed_attitude);
    assert(vehicle_state_on_msp(&input, 2250u));
    const VehicleState *state = vehicle_state_get();
    near(state->throttle, frozen.throttle);
    near(state->steering, frozen.steering);
    near(state->aux_page, frozen.aux_page);
    near(state->roll_deg, frozen.roll_deg);
    near(state->pitch_deg, frozen.pitch_deg);
    near(state->heading_deg, frozen.heading_deg);
    assert(state->link == LINK_LOST);

    input = frame(MSP_ANALOG, analog, sizeof analog);
    assert(vehicle_state_on_msp(&input, 2300u));
    input = frame(MSP_RAW_GPS, gps, sizeof gps);
    assert(vehicle_state_on_msp(&input, 2350u));
    state = vehicle_state_get();
    near(state->battery_v, 12.6f);
    assert(state->rssi == 777u);
    assert(state->gps_sats == 12u);
    assert(state->link == LINK_LOST);
}

static void test_stale_loss_freeze_and_continuous_recovery(void)
{
    const uint8_t forward[] = {
        0xdc, 0x05, 0xdc, 0x05, 0xd0, 0x07, 0xdc, 0x05,
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05
    };
    const uint8_t reverse[] = {
        0xdc, 0x05, 0xdc, 0x05, 0xe8, 0x03, 0xdc, 0x05,
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05
    };
    vehicle_state_init();
    MspFrame input = frame(MSP_RC, forward, sizeof forward);
    assert(vehicle_state_on_msp(&input, 100u));
    near(vehicle_state_get()->throttle, 1.0f);
    vehicle_state_tick(600u);
    assert(vehicle_state_get()->link == LINK_STALE);
    vehicle_state_tick(2100u);
    assert(vehicle_state_get()->link == LINK_LOST);

    input = frame(MSP_RC, reverse, sizeof reverse);
    assert(vehicle_state_on_msp(&input, 2200u));
    assert(vehicle_state_get()->link == LINK_LOST);
    near(vehicle_state_get()->throttle, 1.0f);
    assert(vehicle_state_on_msp(&input, 2450u));
    assert(vehicle_state_on_msp(&input, 2950u)); /* a frame gap breaks continuity */
    assert(vehicle_state_get()->link == LINK_LOST);
    assert(vehicle_state_on_msp(&input, 3000u));
    assert(vehicle_state_on_msp(&input, 3250u));
    assert(vehicle_state_on_msp(&input, 3449u));
    assert(vehicle_state_get()->link == LINK_LOST);
    assert(vehicle_state_on_msp(&input, 3450u));
    assert(vehicle_state_get()->link == LINK_OK);
    near(vehicle_state_get()->throttle, 0.3f);
}

void test_vehicle_state(void)
{
    test_lighting_rc_validity_initializes_safe();
    test_decodes_supported_frames();
    test_decodes_lighting_aux_channels();
    test_clamps_lighting_aux_channels();
    test_short_rc_frame_resets_lighting_aux_channels_to_safe_value();
    test_short_rc_frame_clears_lighting_aux_while_lost();
    test_lighting_rc_expires_independently_across_tick_wrap();
    test_channel_mapping_filter_and_deadband();
    test_malformed_is_atomic();
    test_malformed_gps_does_not_advance_recovery();
    test_all_fast_values_freeze_while_lost();
    test_stale_loss_freeze_and_continuous_recovery();
}
