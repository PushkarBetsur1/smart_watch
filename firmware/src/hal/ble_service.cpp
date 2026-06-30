#include "ble_service.h"
#include "../config.h"
#include <bluefruit.h>

static BLEService fitness_service("00001000-0000-1000-8000-00805f9b34fb");
static BLECharacteristic chr_steps("00001001-0000-1000-8000-00805f9b34fb");
static BLECharacteristic chr_battery("00001002-0000-1000-8000-00805f9b34fb");
static BLECharacteristic chr_workout("00001003-0000-1000-8000-00805f9b34fb");
static BLECharacteristic chr_time_sync("00001004-0000-1000-8000-00805f9b34fb");
static BLECharacteristic chr_settings("00001005-0000-1000-8000-00805f9b34fb");

static BLEDis ble_dis;

static void (*time_sync_cb)(const DateTime &dt) = nullptr;

static void time_sync_write_callback(uint16_t conn_hdl, BLECharacteristic *chr, uint8_t *data, uint16_t len) {
    (void)conn_hdl;
    (void)chr;
    if (len == sizeof(DateTime) && time_sync_cb) {
        DateTime dt;
        memcpy(&dt, data, sizeof(DateTime));
        time_sync_cb(dt);
    }
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
    chr_time_sync.setFixedLen(sizeof(DateTime));
    chr_time_sync.setWriteCallback(time_sync_write_callback);
    chr_time_sync.begin();

    chr_settings.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE);
    chr_settings.setPermission(SECMODE_OPEN, SECMODE_OPEN);
    chr_settings.setFixedLen(5);
    chr_settings.begin();

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
