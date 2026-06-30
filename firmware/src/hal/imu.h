#pragma once

#include <stdint.h>

struct ImuData {
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
};

bool imu_init();
bool imu_read(ImuData &data);
float imu_accel_magnitude(const ImuData &data);
void imu_sleep();
void imu_wake();
uint32_t imu_get_hardware_steps();
