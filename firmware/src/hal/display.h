#pragma once

#include <stdint.h>
#include <stdbool.h>

bool display_init();
void display_clear();
void display_update();
void display_set_brightness(uint8_t level);
void display_on();
void display_off();

void display_draw_text(int16_t x, int16_t y, const char *text, uint8_t size);
void display_draw_centered_text(int16_t y, const char *text, uint8_t size);
void display_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void display_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h);
void display_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h);
void display_draw_progress_bar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t percent);
