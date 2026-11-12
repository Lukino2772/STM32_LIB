#ifndef OLED_H
#define OLED_H
#include "main.h"
#include <stdint.h>
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
typedef struct { I2C_HandleTypeDef *i2c; uint8_t address; uint8_t buffer[OLED_WIDTH*OLED_HEIGHT/8]; } OLED_Handle;
HAL_StatusTypeDef OLED_Init(OLED_Handle*,I2C_HandleTypeDef*,uint8_t address);
HAL_StatusTypeDef OLED_Update(OLED_Handle*);
void OLED_Clear(OLED_Handle*);
void OLED_DrawPixel(OLED_Handle*,uint8_t,uint8_t,uint8_t);
void OLED_DrawChar(OLED_Handle*,uint8_t,uint8_t,char);
void OLED_DrawString(OLED_Handle*,uint8_t,uint8_t,const char*);
#endif