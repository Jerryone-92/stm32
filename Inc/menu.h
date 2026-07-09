/*
 * menu.h
 *
 *  Created on: 2026年7月5日
 *      Author: Sinme
 */

#ifndef MENU_H
#define MENU_H

#include <stdint.h>

extern void Menu_Init(void);
extern void Menu_Process(int8_t enc_diff, uint8_t btn_pressed);
extern void Menu_Display(void);

#endif /* MENU_H */
