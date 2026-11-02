#ifndef MAZE_IMU_H
#define MAZE_IMU_H
#include <stdint.h>
typedef struct { uint8_t online,id,address; int16_t a[3],g[3]; uint32_t last_good,errors; } Imu;
int imu_init(Imu *s);
int imu_sample(Imu *s, uint32_t now);
#endif
