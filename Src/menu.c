/*
 * menu.c
 *
 *  Created on: 2026年7月5日
 *      Author: Sinme
 */

#include "menu.h"
#include "oled.h"
#include "rtc.h"
#include <stdio.h>
extern volatile uint32_t step_count;  // 来自 main.c

// 菜单状态枚举：每一个状态代表一个界面或设置步骤
typedef enum {
    STATE_MAIN_PAGE = 0,   // 主界面：显示时间
    STATE_MAIN_MENU,       // 主菜单列表
    STATE_SET_HOUR,        // 设置小时
    STATE_SET_MINUTE,      // 设置分钟
    STATE_SET_YEAR,        // 设置年
    STATE_SET_MONTH,       // 设置月
    STATE_SET_DAY,         // 设置日
	STATE_STEP,            // 步数显示页
    STATE_ABOUT            // 关于页
} MenuState_t;

static MenuState_t state = STATE_MAIN_PAGE;          // 当前状态
static const char *menu_items[] = {"Set Time", "Set Date", "Steps", "About"};
#define MENU_ITEM_COUNT  4
static uint8_t menu_cursor = 0;                      // 主菜单当前选择项索引
static uint8_t temp_hour, temp_min, temp_year, temp_month, temp_day; // 设置时暂存值

/**
 * @brief 菜单初始化，从 RTC 读取当前时间作为设置初始值
 */
void Menu_Init(void) {
    state = STATE_MAIN_PAGE;
    HAL_RTC_WaitForSynchro(&hrtc);   // 等待 RTC 影子寄存器同步
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    temp_hour = sTime.Hours;
    temp_min  = sTime.Minutes;
    temp_year = sDate.Year;
    temp_month = sDate.Month;
    temp_day   = sDate.Date;
}

/**
 * @brief 菜单状态机处理函数
 * @param enc_diff 编码器增量（-1,0,1）
 * @param btn_pressed 按键是否被按下（1 表示按下）
 */
void Menu_Process(int8_t enc_diff, uint8_t btn_pressed) {
    switch (state) {
        case STATE_MAIN_PAGE:
            // 在主界面旋转编码器，直接进入主菜单
            if (enc_diff != 0) {
                state = STATE_MAIN_MENU;
                menu_cursor = 0;
            }
            break;

        case STATE_MAIN_MENU:
            if (enc_diff > 0) {
                menu_cursor = (menu_cursor + 1) % MENU_ITEM_COUNT;       // 原来是 % 3，现在 % 4
            } else if (enc_diff < 0) {
                menu_cursor = (menu_cursor + MENU_ITEM_COUNT - 1) % MENU_ITEM_COUNT;
            }
            if (btn_pressed) {
                switch (menu_cursor) {
                    case 0: state = STATE_SET_HOUR; break;
                    case 1: state = STATE_SET_YEAR; break;
                    case 2: state = STATE_STEP;      break;   // ★ 新增
                    case 3: state = STATE_ABOUT;     break;   // 原 case 2 → 现在的 case 3
                }
            }
            break;

        case STATE_SET_HOUR:
            temp_hour = (temp_hour + enc_diff) % 24; // 旋转调整小时
            if (btn_pressed) state = STATE_SET_MINUTE; // 按下按键进入分钟设置
            break;

        case STATE_SET_MINUTE:
            temp_min = (temp_min + enc_diff) % 60;
            if (btn_pressed) {
                // 保存设置到 RTC
                RTC_TimeTypeDef sTime = {0};
                sTime.Hours = temp_hour;
                sTime.Minutes = temp_min;
                sTime.Seconds = 0;      // 秒清零
                HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
                state = STATE_MAIN_PAGE; // 返回主界面
            }
            break;

        case STATE_SET_YEAR:
            temp_year = (temp_year + enc_diff) % 100;
            if (btn_pressed) state = STATE_SET_MONTH;
            break;

        case STATE_SET_MONTH:
            temp_month = (temp_month + enc_diff) % 13;
            if (temp_month == 0) temp_month = 1;  // 0 变成 1
            if (btn_pressed) state = STATE_SET_DAY;
            break;

        case STATE_SET_DAY:
            temp_day = (temp_day + enc_diff) % 32;
            if (temp_day == 0) temp_day = 1;
            if (btn_pressed) {
                RTC_DateTypeDef sDate = {0};
                sDate.Year  = temp_year;
                sDate.Month = temp_month;
                sDate.Date  = temp_day;
                HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
                state = STATE_MAIN_PAGE;
            }
            break;

        case STATE_ABOUT:
            // 在关于页，旋转或按键都返回主界面
            if (enc_diff != 0 || btn_pressed) {
                state = STATE_MAIN_PAGE;
            }
            break;

        case STATE_STEP:
            if (btn_pressed) {
                state = STATE_MAIN_PAGE;   // 按任意键返回主界面
            }
            break;
    }
}

/**
 * @brief 根据当前状态绘制 OLED 界面
 */
void Menu_Display(void) {
    OLED_ClearBuffer();  // 先清空缓冲区
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    switch (state) {
    case STATE_MAIN_PAGE:
        OLED_ShowString(0, 0, "Time", 1);                              // 标题：Time
        OLED_Printf(0, 16, "%02d:%02d:%02d", sTime.Hours, sTime.Minutes, sTime.Seconds);
        OLED_Printf(0, 32, "20%02d/%02d/%02d", sDate.Year, sDate.Month, sDate.Date);
        OLED_Printf(0, 48, "Steps:%lu", step_count);                   // 步数显示
        break;

        case STATE_MAIN_MENU:
            OLED_ShowString(0, 0, "Main Menu", 1);
            for (uint8_t i = 0; i < 3; i++) {
                uint8_t y = 16 + i * 16;
                if (i == menu_cursor) {
                    OLED_Printf(0, y, ">%s", menu_items[i]); // 箭头指示选中项
                } else {
                    OLED_Printf(8, y, "%s", menu_items[i]);
                }
            }
            break;

        case STATE_SET_HOUR:
            OLED_ShowString(0, 0, "Set Hour:", 1);
            OLED_Printf(0, 16, "%02d", temp_hour);
            break;
        case STATE_SET_MINUTE:
            OLED_ShowString(0, 0, "Set Minute:", 1);
            OLED_Printf(0, 16, "%02d", temp_min);
            break;
        case STATE_SET_YEAR:
            OLED_ShowString(0, 0, "Set Year:", 1);
            OLED_Printf(0, 16, "20%02d", temp_year);
            break;
        case STATE_SET_MONTH:
            OLED_ShowString(0, 0, "Set Month:", 1);
            OLED_Printf(0, 16, "%02d", temp_month);
            break;
        case STATE_SET_DAY:
            OLED_ShowString(0, 0, "Set Day:", 1);
            OLED_Printf(0, 16, "%02d", temp_day);
            break;
        case STATE_ABOUT:
            OLED_ShowString(0, 0, "About", 1);
            OLED_ShowString(0, 16, "Smart Watch", 1);
            OLED_ShowString(0, 32, "V1.0", 1);
            break;

        case STATE_STEP:
            OLED_ShowString(0, 0, "Step Count:", 1);
            OLED_Printf(0, 16, "%lu", step_count);
            OLED_ShowString(0, 48, "Press to ret", 1);
            break;
    }
    OLED_Update(); // 最后将缓冲区内容更新到屏幕
}
