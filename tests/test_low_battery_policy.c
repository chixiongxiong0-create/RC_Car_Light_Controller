#include <assert.h>

#include "ui/low_battery_policy.h"

static LowBatteryPolicy fresh(uint8_t cells)
{
    LowBatteryPolicy policy;
    low_battery_policy_init(&policy, cells);
    return policy;
}

void test_low_battery_policy(void)
{
    LowBatteryPolicy policy = fresh(0u);
    assert(!low_battery_policy_is_configured(&policy));
    assert(low_battery_policy_cell_count(&policy) == 0u);
    assert(!low_battery_policy_update(&policy, 0.0f, false));
    assert(!low_battery_policy_update(&policy, 5.0f, true));

    policy = fresh(1u);
    assert(!low_battery_policy_is_configured(&policy));
    policy = fresh(7u);
    assert(!low_battery_policy_is_configured(&policy));

    policy = fresh(2u);
    assert(low_battery_policy_is_configured(&policy));
    assert(low_battery_policy_cell_count(&policy) == 2u);
    assert(low_battery_policy_update(&policy, 7.0f, true));
    assert(low_battery_policy_update(&policy, 7.39f, true));
    assert(!low_battery_policy_update(&policy, 7.40f, true));

    policy = fresh(3u);
    assert(low_battery_policy_update(&policy, 10.5f, true));
    assert(low_battery_policy_update(&policy, 10.51f, true));
    assert(!low_battery_policy_update(&policy, 11.10f, true));

    policy = fresh(4u);
    assert(low_battery_policy_update(&policy, 14.0f, true));
    assert(low_battery_policy_update(&policy, 14.79f, true));
    assert(!low_battery_policy_update(&policy, 14.80f, true));

    policy = fresh(5u);
    assert(low_battery_policy_update(&policy, 17.5f, true));
    assert(!low_battery_policy_update(&policy, 18.50f, true));

    policy = fresh(6u);
    assert(low_battery_policy_update(&policy, 21.0f, true));
    assert(!low_battery_policy_update(&policy, 22.20f, true));
    assert(!low_battery_policy_update(&policy, 0.0f, false));
    assert(low_battery_policy_update(&policy, 21.0f, true));
    assert(low_battery_policy_update(&policy, -1.0f, true));
}
