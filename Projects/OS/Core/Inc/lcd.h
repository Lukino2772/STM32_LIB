#ifndef LCD_H
#define LCD_H

#include "main.h"
#include <stdint.h>

#define LCD_WIDTH  240
#define LCD_HEIGHT 320

#define LCD_BLACK   0x0000
#define LCD_WHITE   0xFFFF
#define LCD_RED     0xF800
#define LCD_GREEN   0x07E0
#define LCD_BLUE    0x001F
#define LCD_YELLOW  0xFFE0
#define LCD_CYAN    0x07FF
#define LCD_MAGENTA 0xF81F

extern uint16_t lcd_id;   /* detected controller ID, for debugging */

void LCD_Init(void);
void LCD_FillScreen(uint16_t color);
void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void LCD_FillRectangle(uint16_t x, uint16_t y,
                       uint16_t width, uint16_t height,
                       uint16_t color);

#endif
