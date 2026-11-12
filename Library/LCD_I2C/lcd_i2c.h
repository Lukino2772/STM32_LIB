#ifndef LCD_I2C_H
#define LCD_I2C_H
#include "main.h"
#include <stdint.h>
typedef struct {
 I2C_HandleTypeDef *hi2c; uint8_t address; uint8_t columns, rows;
 uint8_t backlight; uint8_t display_control; uint8_t display_mode;
} LCD_I2C_Handle;
HAL_StatusTypeDef LCD_I2C_Init(LCD_I2C_Handle *lcd, I2C_HandleTypeDef *hi2c, uint8_t address, uint8_t cols, uint8_t rows);
HAL_StatusTypeDef LCD_I2C_Clear(LCD_I2C_Handle *lcd);
HAL_StatusTypeDef LCD_I2C_SetCursor(LCD_I2C_Handle *lcd, uint8_t col, uint8_t row);
HAL_StatusTypeDef LCD_I2C_Print(LCD_I2C_Handle *lcd, const char *text);
HAL_StatusTypeDef LCD_I2C_Backlight(LCD_I2C_Handle *lcd, uint8_t on);
#endif