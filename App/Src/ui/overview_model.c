#include "ui/overview_model.h"

#include <stdio.h>
#include <string.h>

enum { FIELD_STALE_MS = 1500u };

static bool fresh(bool valid, uint32_t now_ms, uint32_t then_ms,
                  LinkState link)
{
    return valid && link == LINK_OK &&
           (uint32_t)(now_ms - then_ms) < FIELD_STALE_MS;
}

void overview_model_from_state(const VehicleState *state, uint32_t now_ms,
                               OverviewModel *out)
{
    if (out == NULL) return;
    memset(out, 0, sizeof(*out));
    if (state == NULL) {
        strcpy(out->link, "START");
        strcpy(out->battery, "--");
        strcpy(out->throttle, "--");
        strcpy(out->steering, "--");
        strcpy(out->motion, "UNKNOWN");
        strcpy(out->pitch, "--");
        strcpy(out->roll, "--");
        strcpy(out->gps, "--");
        strcpy(out->armed, "UNKNOWN");
        return;
    }

    const bool rc = fresh(state->rc_valid, now_ms, state->last_rc_ms,
                          state->link);
    const bool attitude = fresh(state->attitude_valid, now_ms,
                                state->last_attitude_ms, state->link);
    const bool battery = fresh(state->battery_valid, now_ms,
                               state->last_analog_ms, state->link);
    const bool gps = fresh(state->gps_valid, now_ms, state->last_gps_ms,
                           state->link);
    const bool status = fresh(state->status_valid, now_ms,
                              state->last_status_ms, state->link);

    if (battery) snprintf(out->battery, sizeof out->battery, "%.1fV",
                          (double)state->battery_v);
    else strcpy(out->battery, "--");
    if (rc) {
        snprintf(out->throttle, sizeof out->throttle, "%+d%%",
                 (int)(state->throttle * 100.0f));
        snprintf(out->steering, sizeof out->steering, "%+d%%",
                 (int)(state->steering * 100.0f));
        strcpy(out->motion, state->throttle > 0.03f ? "FORWARD" :
                            state->throttle < -0.03f ? "REVERSE" : "HOLD");
    } else {
        strcpy(out->throttle, "--");
        strcpy(out->steering, "--");
        strcpy(out->motion, "UNKNOWN");
    }
    if (attitude) {
        snprintf(out->pitch, sizeof out->pitch, "%+.1f",
                 (double)state->pitch_deg);
        snprintf(out->roll, sizeof out->roll, "%+.1f",
                 (double)state->roll_deg);
    } else {
        strcpy(out->pitch, "--");
        strcpy(out->roll, "--");
    }
    if (gps) snprintf(out->gps, sizeof out->gps, "%u",
                      (unsigned)state->gps_sats);
    else strcpy(out->gps, "--");
    strcpy(out->armed, status ? (state->armed ? "ARMED" : "SAFE") : "UNKNOWN");
    switch (state->link) {
    case LINK_OK: strcpy(out->link, "LINK OK"); break;
    case LINK_STALE: strcpy(out->link, "STALE"); break;
    case LINK_LOST: strcpy(out->link, "LINK LOST"); break;
    default: strcpy(out->link, "START"); break;
    }
}
