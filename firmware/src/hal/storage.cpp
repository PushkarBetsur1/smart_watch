#include "storage.h"
#include "../config.h"
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

using namespace Adafruit_LittleFS_Namespace;

static File settings_file(InternalFS);
static File workout_file(InternalFS);
static File steps_file(InternalFS);
static File datetime_file(InternalFS);

bool storage_init() {
    InternalFS.begin();
    return true;
}

bool storage_save_settings(const UserSettings &settings) {
    if (settings_file.open(STORAGE_FILENAME, FILE_O_WRITE)) {
        settings_file.seek(0);
        settings_file.write((const uint8_t *)&settings, sizeof(UserSettings));
        settings_file.close();
        return true;
    }
    return false;
}

bool storage_load_settings(UserSettings &settings) {
    if (settings_file.open(STORAGE_FILENAME, FILE_O_READ)) {
        if (settings_file.read((uint8_t *)&settings, sizeof(UserSettings)) == sizeof(UserSettings)) {
            settings_file.close();
            return true;
        }
        settings_file.close();
    }
    settings.stride_cm = DEFAULT_STRIDE_CM;
    settings.weight_kg = DEFAULT_WEIGHT_KG;
    settings.height_cm = DEFAULT_HEIGHT_CM;
    settings.screen_brightness = 128;
    settings.screen_timeout_ms = SCREEN_TIMEOUT_MS;
    return false;
}

bool storage_save_daily_steps(const DailySteps &steps) {
    if (steps_file.open(STEPS_FILENAME, FILE_O_WRITE)) {
        steps_file.seek(0);
        steps_file.write((const uint8_t *)&steps, sizeof(DailySteps));
        steps_file.close();
        return true;
    }
    return false;
}

bool storage_load_daily_steps(DailySteps &steps) {
    if (steps_file.open(STEPS_FILENAME, FILE_O_READ)) {
        if (steps_file.read((uint8_t *)&steps, sizeof(DailySteps)) == sizeof(DailySteps)) {
            steps_file.close();
            return true;
        }
        steps_file.close();
    }
    memset(&steps, 0, sizeof(DailySteps));
    return false;
}

bool storage_save_workout(const WorkoutRecord &record) {
    if (workout_file.open(WORKOUT_FILENAME, FILE_O_WRITE)) {
        workout_file.seek(0, SeekEnd);
        workout_file.write((const uint8_t *)&record, sizeof(WorkoutRecord));
        workout_file.close();
        return true;
    }
    return false;
}

uint8_t storage_load_workouts(WorkoutRecord *records, uint8_t max_count) {
    uint8_t count = 0;
    if (workout_file.open(WORKOUT_FILENAME, FILE_O_READ)) {
        while (count < max_count) {
            if (workout_file.read((uint8_t *)&records[count], sizeof(WorkoutRecord)) != sizeof(WorkoutRecord)) {
                break;
            }
            count++;
        }
        workout_file.close();
    }
    return count;
}

bool storage_save_datetime(uint32_t epoch) {
    File dt_file(InternalFS);
    if (dt_file.open("/datetime.bin", FILE_O_WRITE)) {
        dt_file.seek(0);
        dt_file.write((const uint8_t *)&epoch, sizeof(uint32_t));
        dt_file.close();
        return true;
    }
    return false;
}

uint32_t storage_load_datetime() {
    uint32_t epoch = 0;
    File dt_file(InternalFS);
    if (dt_file.open("/datetime.bin", FILE_O_READ)) {
        dt_file.read((uint8_t *)&epoch, sizeof(uint32_t));
        dt_file.close();
    }
    return epoch;
}

bool storage_format() {
    InternalFS.format();
    return storage_init();
}
