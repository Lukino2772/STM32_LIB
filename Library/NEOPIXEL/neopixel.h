#ifndef NEOPIXEL_H
#define NEOPIXEL_H
#include "main.h"
#include <stdint.h>
typedef struct { TIM_HandleTypeDef *timer; uint32_t channel; uint32_t *dma_buffer; uint16_t led_count; uint16_t buffer_len; uint32_t zero_compare,one_compare; uint8_t brightness; } NeoPixel_Handle;
HAL_StatusTypeDef NeoPixel_Init(NeoPixel_Handle*,TIM_HandleTypeDef*,uint32_t,uint16_t,uint32_t*,uint16_t,uint32_t,uint32_t);
void NeoPixel_SetPixel(NeoPixel_Handle*,uint16_t,uint8_t,uint8_t,uint8_t);
void NeoPixel_Clear(NeoPixel_Handle*);
HAL_StatusTypeDef NeoPixel_Show(NeoPixel_Handle*);
#endif