#pragma once

#include <stdbool.h>
#include <stdint.h>

#define BATTERY_CELL_LOW_ENTER_V    3.50f
#define BATTERY_CELL_LOW_EXIT_V     3.70f
#define BATTERY_MIN_CELLS           2u
#define BATTERY_MAX_CELLS           6u

typedef struct {
    uint8_t cell_count;
    bool low;
} LowBatteryPolicy;

void low_battery_policy_init(LowBatteryPolicy *policy, uint8_t configured_cells);
bool low_battery_policy_update(LowBatteryPolicy *policy, float battery_v,
                               bool voltage_valid);
bool low_battery_policy_is_configured(const LowBatteryPolicy *policy);
uint8_t low_battery_policy_cell_count(const LowBatteryPolicy *policy);
