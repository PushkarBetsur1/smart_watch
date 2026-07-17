#pragma once

#include <stdint.h>

struct DateTime {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  weekday;   // 0=Mon, 6=Sun
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
};

void datetime_init();
void datetime_tick();
void datetime_set(const DateTime &dt);
DateTime datetime_get();
bool datetime_is_valid();
uint32_t datetime_to_epoch(const DateTime &dt);
DateTime datetime_from_epoch(uint32_t epoch);
const char* datetime_weekday_str(uint8_t weekday);
const char* datetime_month_str(uint8_t month);
bool datetime_is_leap_year(uint16_t year);
uint8_t datetime_days_in_month(uint8_t month, uint16_t year);
