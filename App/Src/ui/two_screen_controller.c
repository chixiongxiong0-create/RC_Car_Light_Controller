#include "ui/two_screen_controller.h"

enum { MANUAL_FACE_MS = 10000u };

static FaceMood cycle_mood(FaceMood mood, bool left)
{
    if (mood != FACE_IDLE && mood != FACE_FOCUSED && mood != FACE_REVERSE) {
        mood = FACE_IDLE;
    }
    if (left) {
        return mood == FACE_IDLE ? FACE_FOCUSED :
               mood == FACE_FOCUSED ? FACE_REVERSE : FACE_IDLE;
    }
    return mood == FACE_IDLE ? FACE_REVERSE :
           mood == FACE_REVERSE ? FACE_FOCUSED : FACE_IDLE;
}

void two_screen_init(TwoScreenController *controller)
{
    *controller = (TwoScreenController){0};
    controller->main = MAIN_FACE;
    controller->info = INFO_OVERVIEW;
    controller->auto_mood = FACE_IDLE;
    controller->manual_mood = FACE_IDLE;
}

void two_screen_select_main(TwoScreenController *controller, MainScreen main)
{
    if (main == MAIN_FACE || main == MAIN_INFO) controller->main = main;
}

void two_screen_swipe(TwoScreenController *controller, bool left, uint32_t now_ms)
{
    if (controller->main == MAIN_INFO) {
        controller->info = left ? INFO_DIAGNOSTICS : INFO_OVERVIEW;
        return;
    }
    const FaceMood current = controller->manual ? controller->manual_mood :
                             controller->auto_mood;
    controller->manual_mood = cycle_mood(current, left);
    controller->manual_at_ms = now_ms;
    controller->manual = true;
}

void two_screen_button(TwoScreenController *controller, bool touch_available)
{
    if (!touch_available) {
        controller->main = controller->main == MAIN_FACE ? MAIN_INFO : MAIN_FACE;
    }
}

void two_screen_tick(TwoScreenController *controller, uint32_t now_ms,
                     const VehicleState *state, bool low_battery)
{
    if (controller->manual &&
        (uint32_t)(now_ms - controller->manual_at_ms) >= MANUAL_FACE_MS) {
        controller->manual = false;
    }
    controller->auto_mood = face_model_from_state(state, low_battery).mood;
}

TwoScreenView two_screen_view(const TwoScreenController *controller)
{
    FaceMood mood = controller->manual ? controller->manual_mood :
                    controller->auto_mood;
    if (controller->auto_mood == FACE_LINK_LOST ||
        controller->auto_mood == FACE_LOW_BATTERY) {
        mood = controller->auto_mood;
    }
    return (TwoScreenView){controller->main, controller->info, mood,
                           controller->manual};
}
