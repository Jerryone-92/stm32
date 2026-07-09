#include "oled.h"
#include "spi.h"
#include "font.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// 引脚宏（根据你的 User Label 调整，这里使用常见命名）
#define OLED_RST_LOW()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET)
#define OLED_RST_HIGH()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET)
#define OLED_DC_LOW()    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET)
#define OLED_DC_HIGH()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET)
#define OLED_CS_LOW()    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
#define OLED_CS_HIGH()   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)

static uint8_t OLED_GRAM[128][8];
extern SPI_HandleTypeDef hspi1;

// ---------- SPI 基本操作 ----------
static void OLED_WriteByte(uint8_t dat, uint8_t cmd) {
    OLED_CS_LOW();
    if (cmd == 0) OLED_DC_LOW();
    else          OLED_DC_HIGH();
    HAL_SPI_Transmit(&hspi1, &dat, 1, 10);
    OLED_CS_HIGH();
}

// ---------- 初始化 ----------
void OLED_Init(void) {
    OLED_RST_HIGH(); HAL_Delay(100);
    OLED_RST_LOW();  HAL_Delay(100);
    OLED_RST_HIGH(); HAL_Delay(100);

    OLED_WriteByte(0xAE, 0); // display off
    OLED_WriteByte(0x20, 0); // set memory addressing mode
    OLED_WriteByte(0x00, 0); // horizontal addressing mode
    OLED_WriteByte(0xB0, 0); // set page start address
    OLED_WriteByte(0xC0, 0); // COM output scan direction (可调: 0xC8 或 0xC0)
    OLED_WriteByte(0x00, 0); // set low column address
    OLED_WriteByte(0x10, 0); // set high column address
    OLED_WriteByte(0x40, 0); // set display start line
    OLED_WriteByte(0x81, 0); // set contrast
    OLED_WriteByte(0xFF, 0); // max contrast
    OLED_WriteByte(0xA1, 0); // segment remap (可调: 0xA1 或 0xA0)
    OLED_WriteByte(0xA6, 0); // normal display
    OLED_WriteByte(0xA8, 0); // set multiplex ratio
    OLED_WriteByte(0x3F, 0); // 1/64 duty
    OLED_WriteByte(0xA4, 0); // output follows RAM
    OLED_WriteByte(0xD3, 0); // set display offset
    OLED_WriteByte(0x00, 0); // no offset
    OLED_WriteByte(0xD5, 0); // set osc frequency
    OLED_WriteByte(0xF0, 0);
    OLED_WriteByte(0xD9, 0); // set pre-charge period
    OLED_WriteByte(0x22, 0);
    OLED_WriteByte(0xDA, 0); // set COM pins
    OLED_WriteByte(0x12, 0);
    OLED_WriteByte(0xDB, 0); // set VCOMH
    OLED_WriteByte(0x20, 0);
    OLED_WriteByte(0x8D, 0); // charge pump
    OLED_WriteByte(0x14, 0);
    OLED_WriteByte(0xAF, 0); // display on

    OLED_Clear();
    OLED_Update();
}

void OLED_Clear(void) {
    memset(OLED_GRAM, 0x00, sizeof(OLED_GRAM));
}

void OLED_ClearBuffer(void) {
    OLED_Clear();
}

void OLED_Update(void) {
    for (uint8_t page = 0; page < 8; page++) {
        OLED_WriteByte(0xB0 + page, 0);
        OLED_WriteByte(0x00, 0);
        OLED_WriteByte(0x10, 0);
        for (uint8_t x = 0; x < 128; x++) {
            OLED_WriteByte(OLED_GRAM[x][page], 1);
        }
    }
}

// ---------- 字节反转（解决位顺序问题）----------
static uint8_t reverse_byte(uint8_t b) {
    b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
    b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
    b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);
    return b;
}

// ---------- 显示 8x16 字符（y 必须为 8 的倍数）----------
void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size) {
    if (x + 8 > 128 || y + 16 > 64) return;
    uint8_t index = ch - ' ';
    uint8_t page = y / 8;

    for (uint8_t col = 0; col < 8; col++) {
        uint8_t data_upper = Font_8x16[index][col];      // 上半部分（前8字节）
        uint8_t data_lower = Font_8x16[index][col + 8];  // 下半部分（后8字节）

        // 字节反转以适配 SSD1306 的页内位映射
        data_upper = reverse_byte(data_upper);
        data_lower = reverse_byte(data_lower);

        OLED_GRAM[x + col][page]     = data_lower;   // 把下半部分的数据写到上面的页
        OLED_GRAM[x + col][page + 1] = data_upper;   // 把上半部分的数据写到下面的页
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size) {
    while (*str) {
        OLED_ShowChar(x, y, *str++, size);
        x += 8;
        if (x + 8 > 128) {
            x = 0;           // 回到行首
            y += 16;         // 这里的 16 就是你的字模高度
        }
    }
}

void OLED_Printf(uint8_t x, uint8_t y, const char *fmt, ...) {
    char buf[64];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    OLED_ShowString(x, y, buf, 1);
}
