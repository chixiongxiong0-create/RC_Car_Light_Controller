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
    bool manual;
} TwoScreenController;

typedef struct {
    MainScreen main;
    InfoPage info;
    FaceMood mood;
    bool manual;
} TwoScreenView;

void two_screen_init(TwoScreenController *controller);
void two_screen_select_main(TwoScreenController *controller, MainScreen main);
void two_screen_swipe(TwoScreenController *controller, bool left, uint32_t now_ms);
void two_screen_button(TwoScreenController *controller, bool touch_available);
void two_screen_tick(TwoScreenController *controller, uint32_t now_ms,
                     const VehicleState *state, bool low_battery);
TwoScreenView two_screen_view(const TwoScreenController *controller);
