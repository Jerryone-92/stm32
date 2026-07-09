/*
 * oled.h
 *
 *  Created on: 2026年7月5日
 *      Author: Sinme
 */

#ifndef OLED_H
#define OLED_H

#include "main.h"
#include "font.h"

/* 函数外部声明 */
extern void OLED_Init(void);
extern void OLED_Clear(void);
extern void OLED_Update(void);
extern void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size);
extern void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size);
extern void OLED_Printf(uint8_t x, uint8_t y, const char *fmt, ...);
extern void OLED_ClearBuffer(void);

#endif /* OLED_H */
