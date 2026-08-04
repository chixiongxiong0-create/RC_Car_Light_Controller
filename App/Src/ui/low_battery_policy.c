#include "ui/low_battery_policy.h"

void low_battery_policy_init(LowBatteryPolicy *policy, uint8_t configured_cells)
{
    if (policy != 0) {
        policy->cell_count = configured_cells >= BATTERY_MIN_CELLS &&
                             configured_cells <= BATTERY_MAX_CELLS ?
                             configured_cells : 0u;
        policy->low = false;
    }
}

bool low_battery_policy_update(LowBatteryPolicy *policy, float battery_v,
                               bool voltage_valid)
{
    if (policy == 0) {
        return false;
    }
    if (policy->cell_count == 0u) {
        policy->low = false;
        return false;
    }
    if (!voltage_valid || !(battery_v > 0.0f)) {
        return policy->low;
    }

    const float per_cell_v = battery_v / policy->cell_count;
    if (policy->low) {
        if (per_cell_v >= BATTERY_CELL_LOW_EXIT_V) {
            policy->low = false;
        }
    } else if (per_cell_v <= BATTERY_CELL_LOW_ENTER_V) {
        policy->low = true;
    }
    return policy->low;
}

bool low_battery_policy_update_from_vehicle(LowBatteryPolicy *policy,
                                            bool demo_active,
                                            const VehicleState *real_state)
{
    if (policy == 0) {
        return false;
    }
    if (demo_active) {
        policy->low = false;
        return false;
    }
    if (real_state == 0) {
        return low_battery_policy_update(policy, 0.0f, false);
    }
    return low_battery_policy_update(policy, real_state->battery_v,
                                     real_state->battery_valid);
}

bool low_battery_policy_is_configured(const LowBatteryPolicy *policy)
{
    return policy != 0 && policy->cell_count != 0u;
}

uint8_t low_battery_policy_cell_count(const LowBatteryPolicy *policy)
{
    return policy != 0 ? policy->cell_count : 0u;
}
