#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "ui/screen_face.h"

typedef enum { MAIN_FACE, MAIN_INFO } MainScreen;
typedef enum { INFO_OVERVIEW, INFO_DIAGNOSTICS } InfoPage;

typedef struct {
    MainScreen main;
    InfoPage info;
    FaceMood auto_mood;
    FaceMood manual_mood;
    uint32_t manual_at_ms;
    uint32_t next_action_ms;
    uint32_t action_until_ms;
    uint32_t action_at_ms;
    FaceAction action;
    FaceAction last_action;
    FaceMood transient_mood;
    uint32_t transient_until_ms;
    float previous_throttle;
    LinkState previous_link;
    bool have_state;
    bool manual;
} TwoScreenController;

typedef struct {
    MainScreen main;
    InfoPage info;
    FaceMood mood;
    FaceAction action;
    uint32_t action_at_ms;
    bool manual;
} TwoScreenView;

void two_screen_init(TwoScreenController *controller);
void two_screen_select_main(TwoScreenController *controller, MainScreen main);
void two_screen_page_swipe(TwoScreenController *controller, bool left);
void two_screen_face_swipe(TwoScreenController *controller, bool up, uint32_t now_ms);
void two_screen_face_tap(TwoScreenController *controller);
void two_screen_button(TwoScreenController *controller, bool touch_available);
void two_screen_tick(TwoScreenController *controller, uint32_t now_ms,
                     const VehicleState *state, bool low_battery);
TwoScreenView two_screen_view(const TwoScreenController *controller);
