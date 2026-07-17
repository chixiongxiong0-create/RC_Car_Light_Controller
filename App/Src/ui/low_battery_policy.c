#include "ui/low_battery_policy.h"

void low_battery_policy_init(LowBatteryPolicy *policy)
{
    if (policy != 0) {
        policy->cell_count = 0u;
        policy->low = false;
    }
}

bool low_battery_policy_update(LowBatteryPolicy *policy, float battery_v,
                               bool voltage_valid)
{
    if (policy == 0) {
        return false;
    }
    if (!voltage_valid || !(battery_v >= BATTERY_MIN_INFER_V) ||
        battery_v > BATTERY_CELL_MAX_V * BATTERY_MAX_CELLS) {
        return policy->low;
    }

    uint8_t inferred = BATTERY_MIN_CELLS;
    while (inferred <= BATTERY_MAX_CELLS &&
           battery_v > BATTERY_CELL_MAX_V * inferred) {
        ++inferred;
    }
    if (inferred > BATTERY_MAX_CELLS) {
        return policy->low;
    }
    if (inferred > policy->cell_count) {
        policy->cell_count = inferred;
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
