/*
 * mpu6050.h
 *
 *  Created on: 2026年7月7日
 *      Author: Sinme
 */
#ifndef MPU6050_H
#define MPU6050_H

#include "main.h"

#define MPU6050_ADDR  0x68

void MPU6050_Init(void);
HAL_StatusTypeDef MPU6050_ReadAccel(int16_t *ax, int16_t *ay, int16_t *az);

#endif
