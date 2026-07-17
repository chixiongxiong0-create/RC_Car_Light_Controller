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

static void test_decodes_supported_frames(void)
{
    const uint8_t rc_payload[] = {
        0xe8, 0x03, 0xdc, 0x05, 0xd0, 0x07, 0xdc, 0x05,
        0xd0, 0x07, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05
    };
    const uint8_t attitude_payload[] = {0x7b, 0x00, 0x38, 0xff, 0x0e, 0x01};
    const uint8_t analog_payload[] = {123, 0x34, 0x12, 0x4d, 0x03, 0x78, 0x56};
    const uint8_t gps_payload[] = {
        2, 11, 1, 2, 3, 4, 5, 6, 7, 8, 0x34, 0x12, 0x78, 0x56, 0xbc, 0x9a
    };
    const uint8_t status_payload[] = {
        1, 2, 3, 4, 5, 6, 1, 0, 0, 0, 7
    };

    vehicle_state_init();
    MspFrame input = frame(MSP_RC, rc_payload, sizeof rc_payload);
    assert(vehicle_state_on_msp(&input, 100u));
    const VehicleState *state = vehicle_state_get();
    near(state->steering, -1.0f);
    near(state->throttle, 1.0f);
    near(state->aux_page, 1.0f);
    assert(state->last_rc_ms == 100u && state->last_msp_ms == 100u);
    assert(state->link == LINK_OK);

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

    input = frame(MSP_RAW_GPS, gps_payload, sizeof gps_payload);
    assert(vehicle_state_on_msp(&input, 130u));
    assert(vehicle_state_get()->gps_sats == 11u);

    input = frame(MSP_STATUS, status_payload, sizeof status_payload);
    assert(vehicle_state_on_msp(&input, 140u));
    assert(vehicle_state_get()->armed);
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
    vehicle_state_init();
    MspFrame input = frame(MSP_RC, center, sizeof center);
    assert(vehicle_state_on_msp(&input, 1u));
    near(vehicle_state_get()->steering, 0.0f);
    near(vehicle_state_get()->throttle, 0.0f);
    input = frame(MSP_RC, small_offset, sizeof small_offset);
    assert(vehicle_state_on_msp(&input, 2u));
    near(vehicle_state_get()->steering, 0.0f);
    near(vehicle_state_get()->throttle, 0.0f);
}

static void test_malformed_is_atomic(void)
{
    const uint8_t valid[] = {100, 0, 0, 0, 0x22, 0x11, 0, 0};
    const uint8_t bad[] = {250, 0, 0, 0};
    vehicle_state_init();
    MspFrame input = frame(MSP_ANALOG, valid, 7u);
    assert(vehicle_state_on_msp(&input, 10u));
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
        {MSP_RAW_GPS, 15u},
        {MSP_STATUS, 10u}
    };
    const uint8_t zeros[16] = {0};
    for (size_t i = 0; i < sizeof malformed / sizeof malformed[0]; ++i) {
        input = frame(malformed[i].command, zeros, malformed[i].length);
        assert(!vehicle_state_on_msp(&input, 1000u + (uint32_t)i));
        assert(memcmp(&before, vehicle_state_get(), sizeof before) == 0);
    }
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
    test_decodes_supported_frames();
    test_channel_mapping_filter_and_deadband();
    test_malformed_is_atomic();
    test_stale_loss_freeze_and_continuous_recovery();
}
