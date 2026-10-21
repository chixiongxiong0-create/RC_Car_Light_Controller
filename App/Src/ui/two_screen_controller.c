#include "ui/two_screen_controller.h"

enum { MANUAL_FACE_MS = 10000u };

static FaceAction scheduled_action(FaceMood mood, uint32_t slot,
                                   FaceAction previous)
{
    static const FaceAction calm[] = {FACE_ACTION_BLINK, FACE_ACTION_DOUBLE_BLINK,
        FACE_ACTION_SMILE, FACE_ACTION_TONGUE, FACE_ACTION_WINK_LEFT,
        FACE_ACTION_WINK_RIGHT, FACE_ACTION_SCAN, FACE_ACTION_BROW,
        FACE_ACTION_SMIRK};
    static const FaceAction active[] = {FACE_ACTION_BLINK,
        FACE_ACTION_DOUBLE_BLINK, FACE_ACTION_SCAN, FACE_ACTION_BROW};
    static const FaceAction tired[] = {FACE_ACTION_BLINK, FACE_ACTION_DOUBLE_BLINK};
    const FaceAction *pool = calm;
    unsigned count = sizeof calm / sizeof calm[0];
    if (mood == FACE_LOW_BATTERY || mood == FACE_SLEEPY) {
        pool = tired; count = sizeof tired / sizeof tired[0];
    } else if (mood == FACE_FOCUSED || mood == FACE_RAGE || mood == FACE_TURN ||
               mood == FACE_REVERSE || mood == FACE_BRAKE) {
        pool = active; count = sizeof active / sizeof active[0];
    }
    FaceAction action = pool[slot % count];
    if (action == previous) action = pool[(slot + 1u) % count];
    return action;
}

static FaceMood cycle_mood(FaceMood mood, bool left)
{
    static const FaceMood manual[] = {FACE_HAPPY, FACE_SLEEPY, FACE_CONFUSED,
        FACE_ANGRY, FACE_TAUNT, FACE_WINK_LEFT, FACE_WINK_RIGHT, FACE_SURPRISED};
    unsigned i = 0u;
    while (i < sizeof manual / sizeof manual[0] && manual[i] != mood) ++i;
    if (i == sizeof manual / sizeof manual[0]) return manual[0];
    if (left) return manual[(i + 1u) % (sizeof manual / sizeof manual[0])];
    return manual[(i + (sizeof manual / sizeof manual[0]) - 1u) %
                  (sizeof manual / sizeof manual[0])];
}

void two_screen_init(TwoScreenController *controller)
{
    *controller = (TwoScreenController){0};
    controller->main = MAIN_FACE;
    controller->info = INFO_OVERVIEW;
    controller->auto_mood = FACE_IDLE;
    controller->manual_mood = FACE_IDLE;
    controller->next_action_ms = 5000u;
}

void two_screen_select_main(TwoScreenController *controller, MainScreen main)
{
    if (main == MAIN_FACE || main == MAIN_INFO) controller->main = main;
}

void two_screen_page_swipe(TwoScreenController *controller, bool left)
{
    if (left && controller->main == MAIN_FACE) controller->main = MAIN_INFO;
    else if (left && controller->info == INFO_OVERVIEW)
        controller->info = INFO_DIAGNOSTICS;
    else if (!left && controller->main == MAIN_INFO &&
             controller->info == INFO_DIAGNOSTICS)
        controller->info = INFO_OVERVIEW;
    else if (!left && controller->main == MAIN_INFO)
        controller->main = MAIN_FACE;
}

void two_screen_face_swipe(TwoScreenController *controller, bool up, uint32_t now_ms)
{
    const FaceMood current = controller->manual ? controller->manual_mood :
                             FACE_IDLE;
    controller->manual_mood = cycle_mood(current, up);
    controller->manual_at_ms = now_ms;
    controller->manual = true;
}

void two_screen_face_tap(TwoScreenController *controller) { controller->manual = false; }

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
    const FaceMood state_mood = face_model_from_state(state, low_battery).mood;
    if (state != NULL && controller->have_state &&
        controller->previous_link == LINK_LOST && state->link == LINK_OK) {
        controller->transient_mood = FACE_HAPPY;
        controller->transient_until_ms = now_ms + 1300u;
    } else if (state != NULL && controller->have_state &&
               controller->previous_throttle >= 0.35f &&
               state->throttle < 0.10f && state->throttle >= -0.05f) {
        controller->transient_mood = FACE_BRAKE;
        controller->transient_until_ms = now_ms + 1000u;
    }
    controller->auto_mood = state_mood;
    if (!face_mood_is_alert(state_mood) &&
        (int32_t)(controller->transient_until_ms - now_ms) > 0)
        controller->auto_mood = controller->transient_mood;
    if (state != NULL) {
        controller->previous_throttle = state->throttle;
        controller->previous_link = state->link;
        controller->have_state = true;
    }
    if (controller->auto_mood == FACE_LINK_LOST) {
        controller->action = FACE_ACTION_NONE;
        controller->next_action_ms = now_ms + 5000u;
    } else {
        if (controller->action != FACE_ACTION_NONE &&
            (int32_t)(now_ms - controller->action_until_ms) >= 0)
            controller->action = FACE_ACTION_NONE;
        if ((int32_t)(now_ms - controller->next_action_ms) >= 0) {
        const FaceMood effective = controller->manual ? controller->manual_mood :
                                   controller->auto_mood;
        FaceAction next = scheduled_action(effective, now_ms / 5000u,
                                           controller->last_action);
        controller->action = controller->last_action = next;
        controller->action_at_ms = now_ms;
        controller->action_until_ms = now_ms + 900u;
        controller->next_action_ms = now_ms + 5000u;
        }
    }
}

TwoScreenView two_screen_view(const TwoScreenController *controller)
{
    FaceMood mood = controller->manual ? controller->manual_mood :
                    controller->auto_mood;
    if (controller->auto_mood == FACE_LINK_LOST ||
        controller->auto_mood == FACE_LOW_BATTERY ||
        controller->auto_mood == FACE_BRAKE ||
        controller->auto_mood == FACE_HAPPY) {
        mood = controller->auto_mood;
    }
    return (TwoScreenView){controller->main, controller->info, mood,
                           controller->action, controller->action_at_ms,
                           controller->manual};
}
