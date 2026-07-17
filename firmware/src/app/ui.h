#pragma once

#include <stdint.h>

enum UiScreen : uint8_t {
    SCREEN_HOME = 0,
    SCREEN_WORKOUT,
    SCREEN_STATS,
    SCREEN_COUNT
};

enum ButtonEvent : uint8_t {
    BUTTON_NONE = 0,
    BUTTON_SHORT_PRESS,
    BUTTON_LONG_PRESS,
    BUTTON_VERY_LONG_PRESS
};

void ui_init();
void ui_update();
void ui_handle_button(ButtonEvent event);
void ui_set_screen(UiScreen screen);
UiScreen ui_get_screen();
void ui_wake_screen();
bool ui_is_screen_on();
void ui_check_timeout();
