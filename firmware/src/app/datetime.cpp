#include "datetime.h"
#include <string.h>

static DateTime current_dt;

static const char *weekday_names[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
static const char *month_names[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static const uint8_t days_per_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

void datetime_init() {
    memset(&current_dt, 0, sizeof(DateTime));
    current_dt.year = 2024;
    current_dt.month = 1;
    current_dt.day = 1;
}

bool datetime_is_leap_year(uint16_t year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

uint8_t datetime_days_in_month(uint8_t month, uint16_t year) {
    if (month < 1 || month > 12) return 30;
    uint8_t days = days_per_month[month - 1];
    if (month == 2 && datetime_is_leap_year(year)) {
        days = 29;
    }
    return days;
}

void datetime_tick() {
    current_dt.second++;
    if (current_dt.second >= 60) {
        current_dt.second = 0;
        current_dt.minute++;
        if (current_dt.minute >= 60) {
            current_dt.minute = 0;
            current_dt.hour++;
            if (current_dt.hour >= 24) {
                current_dt.hour = 0;
                current_dt.weekday = (current_dt.weekday + 1) % 7;
                current_dt.day++;
                if (current_dt.day > datetime_days_in_month(current_dt.month, current_dt.year)) {
                    current_dt.day = 1;
                    current_dt.month++;
                    if (current_dt.month > 12) {
                        current_dt.month = 1;
                        current_dt.year++;
                    }
                }
            }
        }
    }
}

void datetime_set(const DateTime &dt) {
    current_dt = dt;
}

DateTime datetime_get() {
    return current_dt;
}

uint32_t datetime_to_epoch(const DateTime &dt) {
    uint32_t epoch = 0;
    for (uint16_t y = 1970; y < dt.year; y++) {
        epoch += datetime_is_leap_year(y) ? 366 : 365;
    }
    for (uint8_t m = 1; m < dt.month; m++) {
        epoch += datetime_days_in_month(m, dt.year);
    }
    epoch += dt.day - 1;
    epoch = epoch * 86400UL + dt.hour * 3600UL + dt.minute * 60UL + dt.second;
    return epoch;
}

DateTime datetime_from_epoch(uint32_t epoch) {
    DateTime dt;
    memset(&dt, 0, sizeof(DateTime));

    uint32_t days = epoch / 86400;
    uint32_t remaining = epoch % 86400;

    dt.hour = remaining / 3600;
    remaining %= 3600;
    dt.minute = remaining / 60;
    dt.second = remaining % 60;

    // Day of week: Jan 1 1970 was Thursday (weekday=3)
    dt.weekday = (days + 3) % 7;

    dt.year = 1970;
    while (true) {
        uint16_t year_days = datetime_is_leap_year(dt.year) ? 366 : 365;
        if (days < year_days) break;
        days -= year_days;
        dt.year++;
    }

    dt.month = 1;
    while (true) {
        uint8_t mdays = datetime_days_in_month(dt.month, dt.year);
        if (days < mdays) break;
        days -= mdays;
        dt.month++;
    }
    dt.day = days + 1;

    return dt;
}

const char* datetime_weekday_str(uint8_t weekday) {
    if (weekday > 6) return "???";
    return weekday_names[weekday];
}

const char* datetime_month_str(uint8_t month) {
    if (month < 1 || month > 12) return "???";
    return month_names[month - 1];
}
