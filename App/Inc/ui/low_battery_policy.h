#pragma once

#include <stdbool.h>
#include <stdint.h>

#define BATTERY_CELL_MAX_V          4.25f
#define BATTERY_CELL_LOW_ENTER_V    3.50f
#define BATTERY_CELL_LOW_EXIT_V     3.70f
#define BATTERY_MIN_CELLS           2u
#define BATTERY_MAX_CELLS           6u
#define BATTERY_MIN_INFER_V         6.00f

typedef struct {
    uint8_t cell_count;
    bool low;
} LowBatteryPolicy;

void low_battery_policy_init(LowBatteryPolicy *policy);
bool low_battery_policy_update(LowBatteryPolicy *policy, float battery_v,
                               bool voltage_valid);
