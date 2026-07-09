/*
 * soft_i2c.h
 *
 *  Created on: 2026年7月7日
 *      Author: Sinme
 */

#ifndef SOFT_I2C_H
#define SOFT_I2C_H

#include "main.h"

#define SOFT_I2C_SCL_PIN    GPIO_PIN_8   // PB8
#define SOFT_I2C_SDA_PIN    GPIO_PIN_9   // PB9
#define SOFT_I2C_PORT       GPIOB

void Soft_I2C_Init(void);
uint8_t Soft_I2C_WriteByte(uint8_t addr, uint8_t reg, uint8_t data);
uint8_t Soft_I2C_ReadBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len);

#endif
