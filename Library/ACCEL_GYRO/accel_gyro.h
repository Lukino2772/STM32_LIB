#ifndef ACCEL_GYRO_H
#define ACCEL_GYRO_H
#include "main.h"
#include <stdint.h>
typedef enum { AG_MPU6050=0x68, AG_MPU6500=0x70, AG_MPU9250=0x71, AG_MPU9255=0x73 } AG_Type;
typedef struct { I2C_HandleTypeDef *i2c; uint8_t address; AG_Type type; float ax,ay,az,gx,gy,gz; float accel_scale,gyro_scale; } AG_Handle;
HAL_StatusTypeDef AG_Init(AG_Handle*,I2C_HandleTypeDef*,uint8_t address);
HAL_StatusTypeDef AG_Read(AG_Handle*);
HAL_StatusTypeDef AG_SetRanges(AG_Handle*,uint8_t accel_range,uint8_t gyro_range);
#endif