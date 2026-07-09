/*
 * encoder.c
 *
 *  Created on: 2026年7月5日
 *      Author: Sinme
 */

#include "encoder.h"

extern TIM_HandleTypeDef htim2;     // CubeMX 生成的定时器句柄
static int16_t last_count = 0;      // 上次计数器值

void Encoder_Init(void) {
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);  // 启动编码器模式
    last_count = TIM2->CNT;                          // 记录当前计数值
}

int8_t Encoder_GetDiff(void) {
    int16_t cnt = TIM2->CNT;
    int16_t diff = cnt - last_count;
    last_count = cnt;

    // 四倍频时，编码器旋转一个脉冲产生 4 个计数，我们设定 >=4 就算一次有效转动
    if (diff >= 4) {
        return 1;   // 正向旋转一格
    } else if (diff <= -4) {
        return -1;  // 反向旋转一格
    }
    return 0;       // 无有效转动
}
