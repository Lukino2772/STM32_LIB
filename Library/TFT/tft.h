#ifndef TFT_H
#define TFT_H
#include "main.h"
#include <stdint.h>
typedef enum { TFT_BUS_SPI, TFT_BUS_PARALLEL8, TFT_BUS_PARALLEL16 } TFT_BusType;
typedef enum { TFT_ILI9325, TFT_ILI9341, TFT_ILI9488, TFT_ST7735, TFT_ST7789 } TFT_Controller;
typedef struct {
 TFT_BusType type; SPI_HandleTypeDef *spi;
 void (*write_data)(uint16_t value,uint8_t bits,void *user);
 void (*write_command)(uint8_t command,void *user);
 void (*set_cs)(uint8_t active,void *user); void (*set_dc)(uint8_t data,void *user);
 void (*set_reset)(uint8_t high,void *user); void *user;
 uint16_t width,height; uint8_t madctl,color_order;
} TFT_Handle;
#define TFT_BLACK 0x0000
#define TFT_WHITE 0xFFFF
#define TFT_RED 0xF800
#define TFT_GREEN 0x07E0
#define TFT_BLUE 0x001F
#define TFT_RGB(r,g,b) (uint16_t)((((r)&0xF8)<<8)|(((g)&0xFC)<<3)|((b)>>3))
HAL_StatusTypeDef TFT_Init(TFT_Handle*,TFT_Controller);
void TFT_WriteCommand(TFT_Handle*,uint8_t);
void TFT_WriteData(TFT_Handle*,uint8_t);
void TFT_SetWindow(TFT_Handle*,uint16_t,uint16_t,uint16_t,uint16_t);
void TFT_DrawPixel(TFT_Handle*,uint16_t,uint16_t,uint16_t);
void TFT_FillScreen(TFT_Handle*,uint16_t);
void TFT_FillRect(TFT_Handle*,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t);
#endif