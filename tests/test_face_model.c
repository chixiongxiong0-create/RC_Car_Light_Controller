#include <assert.h>

#include "ui/screen_face.h"

static FaceModel model(float throttle, float steering, LinkState link,
                       bool low_battery)
{
    VehicleState state = {0};
    state.throttle = throttle;
    state.steering = steering;
    state.link = link;
    return face_model_from_state(&state, low_battery);
}

void test_face_model(void)
{
    FaceModel face = model(0.0f, 0.0f, LINK_OK, false);
    assert(face.mood == FACE_IDLE);
    assert(face.gaze_x == 0);
    assert(face.aperture == 35u);

    assert(model(FACE_FOCUSED_THRESHOLD - 0.001f, 0.0f, LINK_OK, false).mood == FACE_IDLE);
    assert(model(FACE_FOCUSED_THRESHOLD, 0.0f, LINK_OK, false).mood == FACE_FOCUSED);
    assert(model(FACE_REVERSE_THRESHOLD + 0.001f, 0.0f, LINK_OK, false).mood == FACE_IDLE);
    assert(model(FACE_REVERSE_THRESHOLD, 0.0f, LINK_OK, false).mood == FACE_REVERSE);
    assert(model(-1.0f, 0.0f, LINK_OK, false).mood == FACE_REVERSE);

    assert(model(-1.0f, 0.0f, LINK_OK, true).mood == FACE_LOW_BATTERY);
    assert(model(-1.0f, 0.0f, LINK_LOST, true).mood == FACE_LINK_LOST);

    assert(model(0.0f, -2.0f, LINK_OK, false).gaze_x == -24);
    assert(model(0.0f, -0.5f, LINK_OK, false).gaze_x == -12);
    assert(model(0.0f, 0.5f, LINK_OK, false).gaze_x == 12);
    assert(model(0.0f, 2.0f, LINK_OK, false).gaze_x == 24);

    assert(model(0.0f, 0.0f, LINK_OK, false).aperture == 35u);
    assert(model(0.5f, 0.0f, LINK_OK, false).aperture == 68u);
    assert(model(-0.5f, 0.0f, LINK_OK, false).aperture == 68u);
    assert(model(1.0f, 0.0f, LINK_OK, false).aperture == 100u);
    assert(model(-2.0f, 0.0f, LINK_OK, false).aperture == 100u);

    FaceBatteryModel battery = face_battery_model(12.6f, true, 3u);
    assert(battery.valid);
    assert(battery.percent == 100u);
    assert(battery.segments == 6u);
    assert(!battery.low);

    battery = face_battery_model(10.5f, true, 3u);
    assert(battery.valid);
    assert(battery.percent == 10u);
    assert(battery.segments == 0u);
    assert(battery.low);

    battery = face_battery_model(11.55f, true, 3u);
    assert(battery.valid);
    assert(battery.percent == 55u);
    assert(battery.segments == 3u);
    assert(!battery.low);

    battery = face_battery_model(25.2f, true, 6u);
    assert(battery.valid);
    assert(battery.percent == 100u);
    assert(battery.segments == 6u);

    battery = face_battery_model(11.1f, false, 3u);
    assert(!battery.valid);
    assert(battery.segments == 0u);

    battery = face_battery_model(11.1f, true, 0u);
    assert(!battery.valid);

    const FaceMotionModel before_wrap = face_motion_model(224u, 100u,
                                                           FACE_FOCUSED);
    const FaceMotionModel after_wrap = face_motion_model(225u, 100u,
                                                          FACE_FOCUSED);
    assert(before_wrap.shake_x - after_wrap.shake_x <= 1);
    assert(after_wrap.shake_x - before_wrap.shake_x <= 1);

    const FaceMotionModel blink_opening = face_motion_model(4210u, 35u,
                                                             FACE_IDLE);
    const FaceMotionModel blink_closed = face_motion_model(4260u, 35u,
                                                            FACE_IDLE);
    const FaceMotionModel blink_reopening = face_motion_model(4310u, 35u,
                                                               FACE_IDLE);
    assert(blink_opening.blink_closure > 0u);
    assert(blink_opening.blink_closure < blink_closed.blink_closure);
    assert(blink_closed.blink_closure == 100u);
    assert(blink_reopening.blink_closure < blink_closed.blink_closure);
}
