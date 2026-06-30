#include "display.h"
#include "../config.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>

static Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI,
                              PIN_OLED_DC, PIN_OLED_RST, PIN_OLED_CS);

bool display_init() {
    if (!oled.begin(SSD1306_SWITCHCAPVCC)) {
        return false;
    }
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextWrap(false);
    oled.display();
    return true;
}

void display_clear() {
    oled.clearDisplay();
}

void display_update() {
    oled.display();
}

void display_set_brightness(uint8_t level) {
    oled.ssd1306_command(SSD1306_SETCONTRAST);
    oled.ssd1306_command(level);
}

void display_on() {
    oled.ssd1306_command(SSD1306_DISPLAYON);
}

void display_off() {
    oled.ssd1306_command(SSD1306_DISPLAYOFF);
}

void display_draw_text(int16_t x, int16_t y, const char *text, uint8_t size) {
    oled.setTextSize(size);
    oled.setCursor(x, y);
    oled.print(text);
}

void display_draw_centered_text(int16_t y, const char *text, uint8_t size) {
    oled.setTextSize(size);
    int16_t x1, y1;
    uint16_t w, h;
    oled.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    oled.setCursor((SCREEN_WIDTH - w) / 2, y);
    oled.print(text);
}

void display_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
    oled.drawLine(x0, y0, x1, y1, SSD1306_WHITE);
}

void display_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h) {
    oled.drawRect(x, y, w, h, SSD1306_WHITE);
}

void display_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h) {
    oled.fillRect(x, y, w, h, SSD1306_WHITE);
}

void display_draw_progress_bar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t percent) {
    if (percent > 100) percent = 100;
    oled.drawRect(x, y, w, h, SSD1306_WHITE);
    int16_t fill_w = (w - 2) * percent / 100;
    if (fill_w > 0) {
        oled.fillRect(x + 1, y + 1, fill_w, h - 2, SSD1306_WHITE);
    }
}
