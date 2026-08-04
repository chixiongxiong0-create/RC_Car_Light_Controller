#include <assert.h>
#include <math.h>
#include <string.h>

#include "ui/low_battery_policy.h"
#include "vehicle_state.h"
#include "vehicle_state_source.h"

enum {
    MSP_RC = 105,
    MSP_ANALOG = 110
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

static void accept(VehicleStateSource *source, const MspFrame *input,
                   uint32_t now_ms)
{
    assert(vehicle_state_on_msp(input, now_ms));
    vehicle_state_source_note_real(source, now_ms);
}

static bool evaluate(LowBatteryPolicy *policy, VehicleStateSource *source,
                     uint32_t now_ms)
{
    const VehicleState *real_state;

    vehicle_state_tick(now_ms);
    real_state = vehicle_state_get();
    vehicle_state_source_tick(source, now_ms, 4u, real_state);
    return low_battery_policy_update_from_vehicle(
        policy, vehicle_state_source_is_demo(source), real_state);
}

static void test_demo_and_rc_only_never_evaluate_blended_voltage(void)
{
    const uint8_t rc_payload[] = {
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05,
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05
    };
    VehicleStateSource source;
    LowBatteryPolicy policy;

    vehicle_state_init();
    vehicle_state_source_init(&source);
    low_battery_policy_init(&policy, 4u);

    assert(!evaluate(&policy, &source, 0u));
    assert(vehicle_state_source_get(&source)->battery_v > 0.0f);
    assert(low_battery_policy_update(&policy, 13.6f, true));
    assert(!low_battery_policy_update_from_vehicle(
        &policy, true, vehicle_state_get()));

    const MspFrame rc = frame(MSP_RC, rc_payload, sizeof rc_payload);
    accept(&source, &rc, 10u);
    assert(!vehicle_state_get()->battery_valid);
    assert(!evaluate(&policy, &source, 10u));
    assert(!evaluate(&policy, &source, 100u));
    assert(!evaluate(&policy, &source, 299u));
    assert(!evaluate(&policy, &source, 300u));
    assert(!evaluate(&policy, &source, 1000u));
}

static void test_delayed_analog_uses_raw_real_voltage_immediately(void)
{
    const uint8_t rc_payload[] = {
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05,
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05
    };
    const uint8_t low_analog[] = {136, 0, 0, 0x20, 0x03, 0, 0};
    VehicleStateSource source;
    LowBatteryPolicy policy;

    vehicle_state_init();
    vehicle_state_source_init(&source);
    low_battery_policy_init(&policy, 4u);
    vehicle_state_source_tick(&source, 0u, 4u, vehicle_state_get());

    MspFrame input = frame(MSP_RC, rc_payload, sizeof rc_payload);
    accept(&source, &input, 10u);
    assert(!evaluate(&policy, &source, 150u));

    input = frame(MSP_ANALOG, low_analog, sizeof low_analog);
    accept(&source, &input, 160u);
    assert(vehicle_state_get()->battery_valid);
    near(vehicle_state_get()->battery_v, 13.6f);
    assert(low_battery_policy_update_from_vehicle(
        &policy, vehicle_state_source_is_demo(&source), vehicle_state_get()));
}

static void test_first_tick_queued_frames_ignore_zero_blend_origin(void)
{
    const uint8_t rc_payload[] = {
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05,
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05
    };
    const uint8_t low_analog[] = {136, 0, 0, 0x20, 0x03, 0, 0};
    VehicleStateSource source;
    LowBatteryPolicy policy;

    vehicle_state_init();
    vehicle_state_source_init(&source);
    low_battery_policy_init(&policy, 4u);

    MspFrame input = frame(MSP_RC, rc_payload, sizeof rc_payload);
    accept(&source, &input, 0u);
    input = frame(MSP_ANALOG, low_analog, sizeof low_analog);
    accept(&source, &input, 0u);

    assert(evaluate(&policy, &source, 0u));
    assert(vehicle_state_source_get(&source)->battery_v == 0.0f);
    near(vehicle_state_get()->battery_v, 13.6f);
    assert(vehicle_state_get()->battery_valid);
}

static void test_first_tick_queued_healthy_analog_never_latches_low(void)
{
    const uint8_t rc_payload[] = {
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05,
        0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05, 0xdc, 0x05
    };
    const uint8_t healthy_analog[] = {152, 0, 0, 0x20, 0x03, 0, 0};
    VehicleStateSource source;
    LowBatteryPolicy policy;

    vehicle_state_init();
    vehicle_state_source_init(&source);
    low_battery_policy_init(&policy, 4u);

    MspFrame input = frame(MSP_RC, rc_payload, sizeof rc_payload);
    accept(&source, &input, 0u);
    input = frame(MSP_ANALOG, healthy_analog, sizeof healthy_analog);
    accept(&source, &input, 0u);

    assert(!evaluate(&policy, &source, 0u));
    assert(!evaluate(&policy, &source, 1u));
    near(vehicle_state_get()->battery_v, 15.2f);
    assert(vehicle_state_get()->battery_valid);
}

int main(void)
{
    test_demo_and_rc_only_never_evaluate_blended_voltage();
    test_delayed_analog_uses_raw_real_voltage_immediately();
    test_first_tick_queued_frames_ignore_zero_blend_origin();
    test_first_tick_queued_healthy_analog_never_latches_low();
    return 0;
}
