#include "imu.h"
#include "../config.h"
#include <Wire.h>
#include <math.h>

#define BMI270_CHIP_ID_REG      0x00
#define BMI270_CHIP_ID_VAL      0x24
#define BMI270_CMD_REG          0xFC
#define BMI270_PWR_CONF_REG     0x7C
#define BMI270_PWR_CTRL_REG     0x7D
#define BMI270_ACC_CONF_REG     0x40
#define BMI270_ACC_RANGE_REG    0x41
#define BMI270_GYR_CONF_REG     0x42
#define BMI270_GYR_RANGE_REG    0x43
#define BMI270_INIT_CTRL_REG    0x59
#define BMI270_INIT_DATA_REG    0x5E
#define BMI270_DATA_8_REG       0x0C
#define BMI270_STEP_CNT_REG     0x1E
#define BMI270_INT_STATUS_REG   0x1C

#define BMI270_CMD_SOFTRESET    0xB6

static float accel_scale = 1.0f;
static float gyro_scale = 1.0f;

static bool write_reg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(BMI270_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

static uint8_t read_reg(uint8_t reg) {
    Wire.beginTransmission(BMI270_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)BMI270_ADDR, (uint8_t)1);
    return Wire.read();
}

static bool read_regs(uint8_t reg, uint8_t *buf, uint8_t len) {
    Wire.beginTransmission(BMI270_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    Wire.requestFrom((uint8_t)BMI270_ADDR, len);
    for (uint8_t i = 0; i < len && Wire.available(); i++) {
        buf[i] = Wire.read();
    }
    return true;
}

bool imu_init() {
    Wire.begin();
    Wire.setClock(400000);

    delay(10);

    // Soft reset
    write_reg(BMI270_CMD_REG, BMI270_CMD_SOFTRESET);
    delay(50);

    // Verify chip ID
    uint8_t chip_id = read_reg(BMI270_CHIP_ID_REG);
    if (chip_id != BMI270_CHIP_ID_VAL) {
        return false;
    }

    // Disable advanced power save to allow config
    write_reg(BMI270_PWR_CONF_REG, 0x00);
    delay(1);

    // Configure accelerometer: 50Hz, normal mode, averaging 4
    write_reg(BMI270_ACC_CONF_REG, 0xA8);

    // Accelerometer range: +/- 4g
    write_reg(BMI270_ACC_RANGE_REG, 0x01);
    accel_scale = 4.0f / 32768.0f;

    // Configure gyroscope: 50Hz, normal mode
    write_reg(BMI270_GYR_CONF_REG, 0xA9);

    // Gyroscope range: +/- 2000 deg/s
    write_reg(BMI270_GYR_RANGE_REG, 0x00);
    gyro_scale = 2000.0f / 32768.0f;

    // Enable accelerometer and gyroscope
    write_reg(BMI270_PWR_CTRL_REG, 0x0E);
    delay(50);

    return true;
}

bool imu_read(ImuData &data) {
    uint8_t buf[12];
    if (!read_regs(BMI270_DATA_8_REG, buf, 12)) {
        return false;
    }

    int16_t raw_ax = (int16_t)(buf[1] << 8 | buf[0]);
    int16_t raw_ay = (int16_t)(buf[3] << 8 | buf[2]);
    int16_t raw_az = (int16_t)(buf[5] << 8 | buf[4]);
    int16_t raw_gx = (int16_t)(buf[7] << 8 | buf[6]);
    int16_t raw_gy = (int16_t)(buf[9] << 8 | buf[8]);
    int16_t raw_gz = (int16_t)(buf[11] << 8 | buf[10]);

    data.accel_x = raw_ax * accel_scale;
    data.accel_y = raw_ay * accel_scale;
    data.accel_z = raw_az * accel_scale;
    data.gyro_x = raw_gx * gyro_scale;
    data.gyro_y = raw_gy * gyro_scale;
    data.gyro_z = raw_gz * gyro_scale;

    return true;
}

float imu_accel_magnitude(const ImuData &data) {
    return sqrtf(data.accel_x * data.accel_x +
                 data.accel_y * data.accel_y +
                 data.accel_z * data.accel_z);
}

void imu_sleep() {
    write_reg(BMI270_PWR_CTRL_REG, 0x00);
    write_reg(BMI270_PWR_CONF_REG, 0x03);
}

void imu_wake() {
    write_reg(BMI270_PWR_CONF_REG, 0x00);
    delay(1);
    write_reg(BMI270_PWR_CTRL_REG, 0x0E);
    delay(50);
}

uint32_t imu_get_hardware_steps() {
    uint8_t buf[4];
    if (!read_regs(BMI270_STEP_CNT_REG, buf, 4)) {
        return 0;
    }
    return (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) |
           ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
}
