#ifndef LCD_H
#define LCD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 240 x 320 TFT LCD display definitions */
#define LCD_WIDTH       240U
#define LCD_HEIGHT      320U

/* RGB565 color definitions */
#define LCD_BLACK       0x0000U
#define LCD_WHITE       0xFFFFU
#define LCD_RED         0xF800U
#define LCD_GREEN       0x07E0U
#define LCD_BLUE        0x001FU
#define LCD_YELLOW      0xFFE0U
#define LCD_CYAN        0x07FFU
#define LCD_MAGENTA     0xF81FU
#define LCD_GRAY        0x8410U

/*
 * LCD driver interface.
 * Implement these functions in lcd.c for the selected display controller
 * and STM32 hardware connections.
 */
void LCD_Init(void);
void LCD_Clear(uint16_t color);
void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void LCD_FillRect(uint16_t x, uint16_t y,
                  uint16_t width, uint16_t height,
                  uint16_t color);
void LCD_DrawChar(uint16_t x, uint16_t y, char character,
                  uint16_t foreground, uint16_t background);
void LCD_DrawString(uint16_t x, uint16_t y, const char *string,
                    uint16_t foreground, uint16_t background);

#ifdef __cplusplus
}
#endif

#endif /* LCD_H */
