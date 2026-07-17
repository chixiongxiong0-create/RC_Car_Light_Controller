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
}
