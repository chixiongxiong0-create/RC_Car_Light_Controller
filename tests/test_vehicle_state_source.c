#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "vehicle_state_source.h"

static void near(float actual, float expected)
{
    if (!(fabsf(actual - expected) < 0.001f)) {
        fprintf(stderr, "near failed: actual=%.4f expected=%.4f\n",
                (double)actual, (double)expected);
    }
    assert(fabsf(actual - expected) < 0.001f);
}

static VehicleState real_sample(void)
{
    return (VehicleState){
        .throttle = 0.8f,
        .steering = -0.6f,
        .aux_page = 0.75f,
        .roll_deg = -12.0f,
        .pitch_deg = 10.0f,
        .heading_deg = 350.0f,
        .battery_v = 15.2f,
        .rssi = 912u,
        .mode_flags = 0x15u,
        .gps_sats = 17u,
        .battery_valid = true,
        .armed = true,
        .link = LINK_OK,
        .last_msp_ms = 42u,
        .last_rc_ms = 43u,
        .last_attitude_ms = 44u,
    };
}

static void test_boot_and_unannounced_samples_stay_in_demo(void)
{
    VehicleStateSource source;
    VehicleState real = real_sample();

    vehicle_state_source_init(&source);
    assert(vehicle_state_source_is_demo(&source));
    vehicle_state_source_tick(&source, 0u, 4u, NULL);
    assert(vehicle_state_source_is_demo(&source));
    assert(vehicle_state_source_get(&source)->link == LINK_STARTING);

    vehicle_state_source_tick(&source, 1000u, 4u, &real);
    assert(vehicle_state_source_is_demo(&source));
    assert(vehicle_state_source_get(&source)->link == LINK_STARTING);
    assert(!vehicle_state_source_get(&source)->armed);
}

static void test_takeover_copies_discrete_real_values_immediately(void)
{
    VehicleStateSource source;
    VehicleState real = real_sample();
    const VehicleState *selected;

    vehicle_state_source_init(&source);
    vehicle_state_source_tick(&source, 0u, 4u, NULL);
    vehicle_state_source_note_real(&source, 0u);
    vehicle_state_source_tick(&source, 0u, 4u, &real);
    selected = vehicle_state_source_get(&source);

    assert(!vehicle_state_source_is_demo(&source));
    assert(selected->aux_page == real.aux_page);
    assert(selected->rssi == real.rssi);
    assert(selected->mode_flags == real.mode_flags);
    assert(selected->gps_sats == real.gps_sats);
    assert(selected->battery_valid == real.battery_valid);
    assert(selected->armed == real.armed);
    assert(selected->link == real.link);
    assert(selected->last_msp_ms == real.last_msp_ms);
    assert(selected->last_rc_ms == real.last_rc_ms);
    assert(selected->last_attitude_ms == real.last_attitude_ms);
}

static void test_takeover_blends_numeric_values_over_300_ms(void)
{
    VehicleStateSource source;
    VehicleState real = real_sample();
    VehicleState demo_at_takeover;
    const VehicleState *selected;

    vehicle_state_source_init(&source);
    vehicle_state_source_tick(&source, 0u, 4u, NULL);
    demo_at_takeover = *vehicle_state_source_get(&source);
    vehicle_state_source_note_real(&source, 0u);

    vehicle_state_source_tick(&source, 0u, 4u, &real);
    selected = vehicle_state_source_get(&source);
    near(selected->throttle, demo_at_takeover.throttle);
    near(selected->heading_deg, demo_at_takeover.heading_deg);

    vehicle_state_source_tick(&source, 150u, 4u, &real);
    selected = vehicle_state_source_get(&source);
    near(selected->throttle, (demo_at_takeover.throttle + real.throttle) * 0.5f);
    near(selected->steering, (demo_at_takeover.steering + real.steering) * 0.5f);
    near(selected->roll_deg, (demo_at_takeover.roll_deg + real.roll_deg) * 0.5f);
    near(selected->pitch_deg, (demo_at_takeover.pitch_deg + real.pitch_deg) * 0.5f);
    near(selected->battery_v, (demo_at_takeover.battery_v + real.battery_v) * 0.5f);
    near(selected->heading_deg, 355.0f);

    vehicle_state_source_tick(&source, 299u, 4u, &real);
    selected = vehicle_state_source_get(&source);
    near(selected->throttle,
         demo_at_takeover.throttle +
             (real.throttle - demo_at_takeover.throttle) * (299.0f / 300.0f));
    assert(source.blending);

    vehicle_state_source_tick(&source, 300u, 4u, &real);
    selected = vehicle_state_source_get(&source);
    near(selected->throttle, real.throttle);
    near(selected->steering, real.steering);
    near(selected->roll_deg, real.roll_deg);
    near(selected->pitch_deg, real.pitch_deg);
    near(selected->heading_deg, real.heading_deg);
    near(selected->battery_v, real.battery_v);
    assert(!source.blending);
}

static void test_evolving_heading_targets_remain_continuous(void)
{
    const struct {
        float first_target;
        float second_target;
        float first_expected;
        float second_expected;
    } cases[] = {
        {179.0f, 181.0f, 88.9033f, 90.5f},
        {181.0f, 179.0f, 271.0967f, 269.5f},
        {350.0f, 10.0f, 355.0333f, 5.0f},
        {10.0f, 350.0f, 4.9667f, 355.0f},
        {179.0f, 180.0f, 88.9033f, 90.0f},
        {181.0f, 180.0f, 271.0967f, 270.0f},
    };

    for (size_t i = 0u; i < sizeof cases / sizeof cases[0]; ++i) {
        VehicleStateSource source;
        VehicleState real = real_sample();

        vehicle_state_source_init(&source);
        vehicle_state_source_tick(&source, 0u, 4u, NULL);
        vehicle_state_source_note_real(&source, 0u);

        real.heading_deg = cases[i].first_target;
        vehicle_state_source_tick(&source, 149u, 4u, &real);
        near(vehicle_state_source_get(&source)->heading_deg,
             cases[i].first_expected);

        real.heading_deg = cases[i].second_target;
        vehicle_state_source_tick(&source, 150u, 4u, &real);
        near(vehicle_state_source_get(&source)->heading_deg,
             cases[i].second_expected);
    }
}

static void test_exact_antipode_uses_stable_positive_tie(void)
{
    VehicleStateSource source;
    VehicleState real = real_sample();

    vehicle_state_source_init(&source);
    vehicle_state_source_tick(&source, 0u, 4u, NULL);
    vehicle_state_source_note_real(&source, 0u);
    real.heading_deg = 180.0f;
    vehicle_state_source_tick(&source, 150u, 4u, &real);

    near(vehicle_state_source_get(&source)->heading_deg, 90.0f);
}

static void test_blend_boundary_survives_uint32_wrap(void)
{
    const uint32_t started_ms = UINT32_MAX - 100u;
    VehicleStateSource source;
    VehicleState real = real_sample();
    float from_throttle;

    vehicle_state_source_init(&source);
    vehicle_state_source_tick(&source, started_ms, 4u, NULL);
    from_throttle = vehicle_state_source_get(&source)->throttle;
    vehicle_state_source_note_real(&source, started_ms);

    vehicle_state_source_tick(&source, started_ms + 299u, 4u, &real);
    near(vehicle_state_source_get(&source)->throttle,
         from_throttle + (real.throttle - from_throttle) * (299.0f / 300.0f));
    assert(source.blending);

    vehicle_state_source_tick(&source, started_ms + 300u, 4u, &real);
    near(vehicle_state_source_get(&source)->throttle, real.throttle);
    assert(!source.blending);
}

static void test_stale_and_lost_link_remain_real_after_takeover(void)
{
    VehicleStateSource source;
    VehicleState real = real_sample();

    vehicle_state_source_init(&source);
    vehicle_state_source_tick(&source, 0u, 4u, NULL);
    vehicle_state_source_note_real(&source, 0u);
    vehicle_state_source_tick(&source, 300u, 4u, &real);
    real.link = LINK_STALE;
    vehicle_state_source_tick(&source, 1000u, 4u, &real);

    assert(!vehicle_state_source_is_demo(&source));
    assert(vehicle_state_source_get(&source)->link == LINK_STALE);

    real.link = LINK_LOST;
    real.armed = false;
    vehicle_state_source_tick(&source, 2000u, 4u, &real);

    assert(!vehicle_state_source_is_demo(&source));
    assert(vehicle_state_source_get(&source)->link == LINK_LOST);
    assert(!vehicle_state_source_get(&source)->armed);
}

int main(void)
{
    test_boot_and_unannounced_samples_stay_in_demo();
    test_takeover_copies_discrete_real_values_immediately();
    test_takeover_blends_numeric_values_over_300_ms();
    test_evolving_heading_targets_remain_continuous();
    test_exact_antipode_uses_stable_positive_tie();
    test_blend_boundary_survives_uint32_wrap();
    test_stale_and_lost_link_remain_real_after_takeover();
    return 0;
}
