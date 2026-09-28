#include <assert.h>
#include <string.h>
#include "ui/overview_model.h"

int main(void)
{
    VehicleState car = {0};
    car.link = LINK_OK;
    car.last_msp_ms = 1000u;
    car.last_rc_ms = 1000u;
    car.last_attitude_ms = 1000u;
    car.last_analog_ms = 1000u;
    car.last_gps_ms = 1000u;
    car.last_status_ms = 1000u;
    car.rc_valid = true;
    car.attitude_valid = true;
    car.gps_valid = true;
    car.status_valid = true;
    car.battery_valid = true;
    car.battery_v = 12.3f;
    car.throttle = 0.50f;
    car.steering = -0.25f;
    car.pitch_deg = 10.0f;
    car.roll_deg = -5.0f;
    car.gps_sats = 8;
    car.armed = true;
    OverviewModel out;
    overview_model_from_state(&car, 1100u, &out);
    assert(strcmp(out.battery, "12.3V") == 0);
    assert(strcmp(out.throttle, "+50%") == 0);
    assert(strcmp(out.steering, "-25%") == 0);
    assert(strcmp(out.motion, "FORWARD") == 0);
    assert(strcmp(out.pitch, "+10.0") == 0);
    assert(strcmp(out.roll, "-5.0") == 0);
    assert(strcmp(out.gps, "8") == 0);
    assert(strcmp(out.armed, "ARMED") == 0);
    assert(strcmp(out.link, "LINK OK") == 0);

    car.battery_valid = false;
    overview_model_from_state(&car, 1100u, &out);
    assert(strcmp(out.battery, "--") == 0);

    car.battery_valid = true;
    overview_model_from_state(&car, 2700u, &out);
    assert(strcmp(out.battery, "--") == 0);
    assert(strcmp(out.throttle, "--") == 0);
    assert(strcmp(out.pitch, "--") == 0);
    assert(strcmp(out.gps, "--") == 0);
    assert(strcmp(out.armed, "UNKNOWN") == 0);

    car.link = LINK_LOST;
    overview_model_from_state(&car, 1100u, &out);
    assert(strcmp(out.throttle, "--") == 0);
    assert(strcmp(out.link, "LINK LOST") == 0);
    return 0;
}
