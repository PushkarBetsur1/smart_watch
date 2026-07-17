#include "imu.h"
#include "../config.h"
#include <LSM6DS3.h>
#include <math.h>

static LSM6DS3 sensor(I2C_MODE, 0x6A);
static bool initialized = false;

bool imu_init() {
    sensor.settings.accelRange = IMU_ACCEL_RANGE;
    sensor.settings.accelSampleRate = IMU_SAMPLE_RATE_HZ;
    sensor.settings.accelBandWidth = 50;
    sensor.settings.gyroRange = 500;
    sensor.settings.gyroSampleRate = IMU_SAMPLE_RATE_HZ;
    sensor.settings.gyroBandWidth = 50;
    sensor.settings.tempEnabled = 0;
    sensor.settings.accelFifoEnabled = 0;
    sensor.settings.gyroFifoEnabled = 0;

    initialized = sensor.begin() == IMU_SUCCESS;
    return initialized;
}

bool imu_read(ImuData &data) {
    if (!initialized) return false;

    data.accel_x = sensor.readFloatAccelX();
    data.accel_y = sensor.readFloatAccelY();
    data.accel_z = sensor.readFloatAccelZ();
    data.gyro_x = sensor.readFloatGyroX();
    data.gyro_y = sensor.readFloatGyroY();
    data.gyro_z = sensor.readFloatGyroZ();

    return true;
}

float imu_accel_magnitude(const ImuData &data) {
    return sqrtf(data.accel_x * data.accel_x +
                 data.accel_y * data.accel_y +
                 data.accel_z * data.accel_z);
}
