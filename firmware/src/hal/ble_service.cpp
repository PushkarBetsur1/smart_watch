#include "ble_service.h"
#include "../config.h"
#include <bluefruit.h>

static BLEService fitness_service("00001000-0000-1000-8000-00805f9b34fb");
static BLECharacteristic chr_steps("00001001-0000-1000-8000-00805f9b34fb");
static BLECharacteristic chr_battery("00001002-0000-1000-8000-00805f9b34fb");
static BLECharacteristic chr_workout("00001003-0000-1000-8000-00805f9b34fb");
static BLECharacteristic chr_time_sync("00001004-0000-1000-8000-00805f9b34fb");
static BLECharacteristic chr_phone_gps("00001006-0000-1000-8000-00805f9b34fb");

static BLEDis ble_dis;

static void (*time_sync_cb)(const DateTime &dt) = nullptr;
static void (*phone_gps_cb)(const PhoneGpsUpdate &update) = nullptr;

static uint16_t read_u16_le(const uint8_t *data) {
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t read_u32_le(const uint8_t *data) {
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static void time_sync_write_callback(uint16_t conn_hdl, BLECharacteristic *chr, uint8_t *data, uint16_t len) {
    (void)conn_hdl;
    (void)chr;
    if (len == 7 && time_sync_cb) {
        DateTime dt = {};
        dt.year = read_u16_le(data);
        dt.month = data[2];
        dt.day = data[3];
        dt.hour = data[4];
        dt.minute = data[5];
        dt.second = data[6];

        if (dt.year < 2024 || dt.year > 2099 || dt.month < 1 || dt.month > 12 ||
            dt.day < 1 || dt.day > datetime_days_in_month(dt.month, dt.year) ||
            dt.hour > 23 || dt.minute > 59 || dt.second > 59) {
            return;
        }
        dt.weekday = datetime_from_epoch(datetime_to_epoch(dt)).weekday;
        time_sync_cb(dt);
    }
}

static void phone_gps_write_callback(uint16_t conn_hdl, BLECharacteristic *chr, uint8_t *data, uint16_t len) {
    (void)conn_hdl;
    (void)chr;
    if (len != 10 || data[0] != 1 || !phone_gps_cb) return;

    PhoneGpsUpdate update;
    update.has_fix = (data[1] & 0x01) != 0;
    update.distance_cm = read_u32_le(data + 2);
    update.speed_cm_per_sec = read_u16_le(data + 6);
    update.accuracy_cm = read_u16_le(data + 8);
    phone_gps_cb(update);
}

static void connect_callback(uint16_t conn_handle) {
    (void)conn_handle;
}

static void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
    (void)conn_handle;
    (void)reason;
    Bluefruit.Advertising.start(0);
}

bool ble_init() {
    Bluefruit.begin();
    Bluefruit.setTxPower(BLE_TX_POWER);
    Bluefruit.setName(DEVICE_NAME);
    Bluefruit.Periph.setConnectCallback(connect_callback);
    Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

    ble_dis.setManufacturer("DIY");
    ble_dis.setModel("SmartWatch V1");
    ble_dis.begin();

    fitness_service.begin();

    chr_steps.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
    chr_steps.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
    chr_steps.setFixedLen(4);
    chr_steps.begin();

    chr_battery.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
    chr_battery.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
    chr_battery.setFixedLen(1);
    chr_battery.begin();

    chr_workout.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
    chr_workout.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
    chr_workout.setFixedLen(sizeof(BleWorkoutData));
    chr_workout.begin();

    chr_time_sync.setProperties(CHR_PROPS_WRITE);
    chr_time_sync.setPermission(SECMODE_NO_ACCESS, SECMODE_OPEN);
    chr_time_sync.setFixedLen(7);
    chr_time_sync.setWriteCallback(time_sync_write_callback);
    chr_time_sync.begin();

    chr_phone_gps.setProperties(CHR_PROPS_WRITE);
    chr_phone_gps.setPermission(SECMODE_NO_ACCESS, SECMODE_OPEN);
    chr_phone_gps.setFixedLen(10);
    chr_phone_gps.setWriteCallback(phone_gps_write_callback);
    chr_phone_gps.begin();

    return true;
}

void ble_start_advertising() {
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addService(fitness_service);
    Bluefruit.Advertising.addName();
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(160, 244);    // 100-152.5ms
    Bluefruit.Advertising.setFastTimeout(30);
    Bluefruit.Advertising.start(0);
}

void ble_stop_advertising() {
    Bluefruit.Advertising.stop();
}

bool ble_is_connected() {
    return Bluefruit.connected();
}

void ble_update_steps(uint32_t steps) {
    if (Bluefruit.connected()) {
        chr_steps.notify32(steps);
    }
}

void ble_update_battery(uint8_t percent) {
    if (Bluefruit.connected()) {
        chr_battery.notify8(percent);
    }
}

void ble_update_workout(const BleWorkoutData &data) {
    if (Bluefruit.connected()) {
        chr_workout.notify((const uint8_t *)&data, sizeof(BleWorkoutData));
    }
}

void ble_set_time_sync_callback(void (*callback)(const DateTime &dt)) {
    time_sync_cb = callback;
}

void ble_set_phone_gps_callback(void (*callback)(const PhoneGpsUpdate &update)) {
    phone_gps_cb = callback;
}
