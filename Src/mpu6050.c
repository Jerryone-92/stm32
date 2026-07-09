/*
 * mpu6050.c
 *
 *  Created on: 2026年7月7日
 *      Author: Sinme
 */

#include "mpu6050.h"
#include "soft_i2c.h"

void MPU6050_Init(void) {
    uint8_t tmp;

    // 1. 复位设备
    Soft_I2C_WriteByte(MPU6050_ADDR, 0x6B, 0x80);
    HAL_Delay(300);

    // 2. 唤醒并选择时钟源 (内部 8MHz)，关闭睡眠
    Soft_I2C_WriteByte(MPU6050_ADDR, 0x6B, 0x00);
    HAL_Delay(100);

    // 3. 再次确认睡眠位清零
    Soft_I2C_ReadBytes(MPU6050_ADDR, 0x6B, &tmp, 1);
    tmp &= ~0x40;  // 清除 SLEEP 位
    Soft_I2C_WriteByte(MPU6050_ADDR, 0x6B, tmp);
    HAL_Delay(50);

    // 4. 确保加速度计所有轴开启 (写 0x00 到 PWR_MGMT_2 寄存器 0x6C)
    Soft_I2C_WriteByte(MPU6050_ADDR, 0x6C, 0x00);
    HAL_Delay(50);

    // 5. 设置加速度量程为 ±2g
    Soft_I2C_WriteByte(MPU6050_ADDR, 0x1C, 0x00);
    HAL_Delay(50);

    // 6. 可选：禁用 FIFO 和中断
    Soft_I2C_WriteByte(MPU6050_ADDR, 0x23, 0x00);
    Soft_I2C_WriteByte(MPU6050_ADDR, 0x38, 0x00);
}

HAL_StatusTypeDef MPU6050_ReadAccel(int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t buf[6];
    // 分字节读取，更稳定
    for (int i = 0; i < 6; i++) {
        if (Soft_I2C_ReadBytes(MPU6050_ADDR, 0x3B + i, &buf[i], 1) != 0) {
            return HAL_ERROR;
        }
    }
    *ax = (int16_t)((buf[0] << 8) | buf[1]);
    *ay = (int16_t)((buf[2] << 8) | buf[3]);
    *az = (int16_t)((buf[4] << 8) | buf[5]);
    return HAL_OK;
}
