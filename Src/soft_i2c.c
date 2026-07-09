/*
 * soft_i2c.c
 *
 *  Created on: 2026年7月7日
 *      Author: Sinme
 */

#include "soft_i2c.h"

// 简单的延时（微秒级，无需精确）
static void delay_us(uint32_t us) {
    for (volatile uint32_t i = 0; i < us * 16; i++) {   // 将 8 改为 16 或更大
        __NOP();
    }
}

// 设置 SDA 为输入（读取用）
static void SDA_Input(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SOFT_I2C_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(SOFT_I2C_PORT, &GPIO_InitStruct);
}

// 设置 SDA 为输出
static void SDA_Output(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SOFT_I2C_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SOFT_I2C_PORT, &GPIO_InitStruct);
}

// 设置 SCL 为输出
static void SCL_Output(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SOFT_I2C_SCL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SOFT_I2C_PORT, &GPIO_InitStruct);
}

void Soft_I2C_Init(void) {
    // 初始状态：SCL 和 SDA 均为高（开漏输出不拉低时外部上拉维持高）
    SDA_Output();
    HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SDA_PIN, GPIO_PIN_SET);
    SCL_Output();
    HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SCL_PIN, GPIO_PIN_SET);
}

// 产生 I2C 起始信号
static void I2C_Start(void) {
    SDA_Output();
    HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SDA_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SCL_PIN, GPIO_PIN_SET);
    delay_us(4);
    HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SDA_PIN, GPIO_PIN_RESET);
    delay_us(4);
    HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SCL_PIN, GPIO_PIN_RESET);
}

// 产生 I2C 停止信号
static void I2C_Stop(void) {
    SDA_Output();
    HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SDA_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SCL_PIN, GPIO_PIN_SET);
    delay_us(4);
    HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SDA_PIN, GPIO_PIN_SET);
    delay_us(4);
}

// 发送一个字节，返回 0 表示收到 ACK，1 表示无 ACK
static uint8_t I2C_WriteByte(uint8_t data) {
    SDA_Output();
    for (uint8_t i = 0; i < 8; i++) {
        if (data & 0x80) {
            HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SDA_PIN, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SDA_PIN, GPIO_PIN_RESET);
        }
        delay_us(2);
        HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SCL_PIN, GPIO_PIN_SET);
        delay_us(4);
        HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SCL_PIN, GPIO_PIN_RESET);
        delay_us(2);
        data <<= 1;
    }
    // 释放 SDA 作为输入，检查 ACK
    SDA_Input();
    delay_us(2);
    HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SCL_PIN, GPIO_PIN_SET);
    delay_us(4);
    uint8_t ack = HAL_GPIO_ReadPin(SOFT_I2C_PORT, SOFT_I2C_SDA_PIN); // 0 表示 ACK
    HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SCL_PIN, GPIO_PIN_RESET);
    delay_us(2);
    return (ack == GPIO_PIN_RESET) ? 0 : 1;
}

// 读取一个字节，发送 ACK 或 NACK
static uint8_t I2C_ReadByte(uint8_t ack) {
    uint8_t data = 0;
    SDA_Input();
    for (uint8_t i = 0; i < 8; i++) {
        delay_us(2);
        HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SCL_PIN, GPIO_PIN_SET);
        delay_us(4);
        data <<= 1;
        if (HAL_GPIO_ReadPin(SOFT_I2C_PORT, SOFT_I2C_SDA_PIN) == GPIO_PIN_SET) {
            data |= 0x01;
        }
        HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SCL_PIN, GPIO_PIN_RESET);
        delay_us(2);
    }
    // 发送 ACK/NACK
    SDA_Output();
    if (ack) {
        HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SDA_PIN, GPIO_PIN_SET);  // NACK
    } else {
        HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SDA_PIN, GPIO_PIN_RESET); // ACK
    }
    delay_us(2);
    HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SCL_PIN, GPIO_PIN_SET);
    delay_us(4);
    HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SCL_PIN, GPIO_PIN_RESET);
    delay_us(2);
    return data;
}

// 对外接口：写寄存器
uint8_t Soft_I2C_WriteByte(uint8_t addr, uint8_t reg, uint8_t data) {
    I2C_Start();
    if (I2C_WriteByte(addr << 1)) { I2C_Stop(); return 1; } // 无 ACK
    if (I2C_WriteByte(reg)) { I2C_Stop(); return 2; }
    if (I2C_WriteByte(data)) { I2C_Stop(); return 3; }
    I2C_Stop();
    return 0; // 成功
}

// 对外接口：读多个字节
uint8_t Soft_I2C_ReadBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len) {
    I2C_Start();
    if (I2C_WriteByte(addr << 1)) { I2C_Stop(); return 1; }
    if (I2C_WriteByte(reg)) { I2C_Stop(); return 2; }
    I2C_Start();
    if (I2C_WriteByte((addr << 1) | 0x01)) { I2C_Stop(); return 3; }
    for (uint8_t i = 0; i < len; i++) {
        data[i] = I2C_ReadByte(i == (len - 1) ? 1 : 0); // 最后字节发 NACK
    }
    I2C_Stop();
    return 0; // 成功
}
