#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "msp/msp_codec.h"

typedef enum {
    LINK_STARTING,
    LINK_OK,
    LINK_STALE,
    LINK_LOST
} LinkState;

typedef struct {
    float throttle;
    float steering;
    float aux_page;
    float aux6;
    float aux7;
    float aux8;
    float aux9;
    float roll_deg;
    float pitch_deg;
    float heading_deg;
    float battery_v;
    uint16_t rssi;
    uint32_t mode_flags;
    uint8_t gps_sats;
    bool battery_valid;
    bool armed;
    LinkState link;
    uint32_t last_msp_ms;
    uint32_t last_rc_ms;
    uint32_t last_attitude_ms;
} VehicleState;

void vehicle_state_init(void);
bool vehicle_state_on_msp(const MspFrame *frame, uint32_t now_ms);
void vehicle_state_tick(uint32_t now_ms);
const VehicleState *vehicle_state_get(void);
