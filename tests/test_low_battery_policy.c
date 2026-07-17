#include <assert.h>

#include "ui/low_battery_policy.h"

static LowBatteryPolicy fresh(void)
{
    LowBatteryPolicy policy;
    low_battery_policy_init(&policy);
    return policy;
}

void test_low_battery_policy(void)
{
    LowBatteryPolicy policy = fresh();
    assert(!low_battery_policy_update(&policy, 0.0f, false));
    assert(!low_battery_policy_update(&policy, 0.0f, true));
    assert(!low_battery_policy_update(&policy, 5.99f, true));
    assert(policy.cell_count == 0u);

    policy = fresh();
    assert(!low_battery_policy_update(&policy, 8.0f, true));
    assert(policy.cell_count == 2u);
    assert(low_battery_policy_update(&policy, 7.0f, true));
    assert(low_battery_policy_update(&policy, 7.39f, true));
    assert(!low_battery_policy_update(&policy, 7.40f, true));

    policy = fresh();
    assert(!low_battery_policy_update(&policy, 8.50f, true));
    assert(policy.cell_count == 2u);
    assert(low_battery_policy_update(&policy, 8.51f, true));
    assert(policy.cell_count == 3u);

    policy = fresh();
    assert(!low_battery_policy_update(&policy, 12.6f, true));
    assert(policy.cell_count == 3u);
    assert(low_battery_policy_update(&policy, 10.5f, true));
    assert(policy.cell_count == 3u);
    assert(low_battery_policy_update(&policy, 10.51f, true));
    assert(!low_battery_policy_update(&policy, 11.10f, true));

    policy = fresh();
    assert(!low_battery_policy_update(&policy, 16.8f, true));
    assert(policy.cell_count == 4u);
    assert(low_battery_policy_update(&policy, 14.0f, true));
    assert(low_battery_policy_update(&policy, 14.79f, true));
    assert(!low_battery_policy_update(&policy, 14.80f, true));

    policy = fresh();
    assert(!low_battery_policy_update(&policy, 8.4f, true));
    assert(policy.cell_count == 2u);
    assert(!low_battery_policy_update(&policy, 12.6f, true));
    assert(policy.cell_count == 3u);
    assert(low_battery_policy_update(&policy, 8.0f, true));
    assert(policy.cell_count == 3u);

    assert(low_battery_policy_update(&policy, -1.0f, true));
    assert(low_battery_policy_update(&policy, 30.0f, true));
    assert(policy.cell_count == 3u);
}
