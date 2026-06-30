#include "ui.h"
#include "datetime.h"
#include "step_counter.h"
#include "activity.h"
#include "workout.h"
#include "../hal/display.h"
#include "../hal/battery.h"
#include "../hal/power.h"
#include "../config.h"
#include <Arduino.h>
#include <stdio.h>

static UiScreen current_screen = SCREEN_HOME;
static bool screen_on = true;
static uint32_t screen_on_time = 0;

void ui_init() {
    current_screen = SCREEN_HOME;
    screen_on = true;
    screen_on_time = millis();
}

static void draw_home_screen() {
    DateTime dt = datetime_get();
    char buf[32];

    // Date line
    snprintf(buf, sizeof(buf), "%s %s %u",
             datetime_weekday_str(dt.weekday),
             datetime_month_str(dt.month),
             dt.day);
    display_draw_centered_text(0, buf, 1);

    // Time (large)
    snprintf(buf, sizeof(buf), "%02u:%02u", dt.hour, dt.minute);
    display_draw_centered_text(14, buf, 3);

    // Steps
    uint32_t steps = step_counter_get_count();
    snprintf(buf, sizeof(buf), "%lu steps", (unsigned long)steps);
    display_draw_centered_text(42, buf, 1);

    // Battery
    uint8_t batt = battery_read_percent();
    snprintf(buf, sizeof(buf), "Batt: %u%%", batt);
    display_draw_centered_text(54, buf, 1);
}

static void draw_workout_screen() {
    WorkoutState state = workout_get_state();
    char buf[32];

    if (state == WORKOUT_STOPPED) {
        display_draw_centered_text(10, "No Workout", 1);
        display_draw_centered_text(30, "Long press", 1);
        display_draw_centered_text(42, "to start", 1);
        return;
    }

    // Activity type
    const char *activity_name = activity_get_name(activity_get_current());
    display_draw_centered_text(0, activity_name, 1);

    if (state == WORKOUT_PAUSED) {
        display_draw_centered_text(0, "[PAUSED]", 1);
    }

    // Elapsed time
    uint32_t elapsed = workout_get_elapsed_sec();
    uint8_t hrs = elapsed / 3600;
    uint8_t mins = (elapsed % 3600) / 60;
    uint8_t secs = elapsed % 60;
    snprintf(buf, sizeof(buf), "%02u:%02u:%02u", hrs, mins, secs);
    display_draw_centered_text(14, buf, 2);

    // Steps in workout
    snprintf(buf, sizeof(buf), "%lu steps", (unsigned long)workout_get_steps());
    display_draw_centered_text(36, buf, 1);

    // Distance
    uint32_t dist = workout_get_distance_m();
    if (dist >= 1000) {
        snprintf(buf, sizeof(buf), "%lu.%02lu km",
                 (unsigned long)(dist / 1000),
                 (unsigned long)((dist % 1000) / 10));
    } else {
        snprintf(buf, sizeof(buf), "%lu m", (unsigned long)dist);
    }
    display_draw_centered_text(48, buf, 1);

    // Calories
    snprintf(buf, sizeof(buf), "%u cal", workout_get_calories());
    display_draw_centered_text(58, buf, 1);
}

static void draw_stats_screen() {
    char buf[32];

    display_draw_centered_text(0, "Today", 1);

    display_draw_line(0, 10, SCREEN_WIDTH, 10);

    uint32_t steps = step_counter_get_count();
    snprintf(buf, sizeof(buf), "Steps: %lu", (unsigned long)steps);
    display_draw_text(4, 14, buf, 1);

    uint16_t cadence = step_counter_get_cadence();
    snprintf(buf, sizeof(buf), "Cadence: %u bpm", cadence);
    display_draw_text(4, 26, buf, 1);

    const char *act = activity_get_name(activity_get_current());
    snprintf(buf, sizeof(buf), "Activity: %s", act);
    display_draw_text(4, 38, buf, 1);

    uint8_t batt = battery_read_percent();
    uint16_t mv = battery_read_mv();
    snprintf(buf, sizeof(buf), "Batt: %u%% (%umV)", batt, mv);
    display_draw_text(4, 50, buf, 1);
}

void ui_update() {
    if (!screen_on) return;

    display_clear();

    switch (current_screen) {
        case SCREEN_HOME:
            draw_home_screen();
            break;
        case SCREEN_WORKOUT:
            draw_workout_screen();
            break;
        case SCREEN_STATS:
            draw_stats_screen();
            break;
        default:
            break;
    }

    display_update();
}

void ui_handle_button(ButtonEvent event) {
    if (!screen_on) {
        if (event != BUTTON_NONE) {
            ui_wake_screen();
        }
        return;
    }

    power_reset_activity_timer();
    screen_on_time = millis();

    switch (event) {
        case BUTTON_SHORT_PRESS:
            current_screen = (UiScreen)((current_screen + 1) % SCREEN_COUNT);
            break;

        case BUTTON_LONG_PRESS:
            if (current_screen == SCREEN_WORKOUT || current_screen == SCREEN_HOME) {
                WorkoutState state = workout_get_state();
                if (state == WORKOUT_STOPPED) {
                    workout_start(step_counter_get_count());
                    current_screen = SCREEN_WORKOUT;
                } else if (state == WORKOUT_ACTIVE) {
                    workout_pause();
                } else if (state == WORKOUT_PAUSED) {
                    workout_stop();
                }
            }
            break;

        default:
            break;
    }
}

void ui_set_screen(UiScreen screen) {
    current_screen = screen;
}

UiScreen ui_get_screen() {
    return current_screen;
}

void ui_wake_screen() {
    screen_on = true;
    screen_on_time = millis();
    display_on();
    power_reset_activity_timer();
}

bool ui_is_screen_on() {
    return screen_on;
}

void ui_check_timeout() {
    if (screen_on && (millis() - screen_on_time > SCREEN_TIMEOUT_MS)) {
        screen_on = false;
        display_off();
    }
}
