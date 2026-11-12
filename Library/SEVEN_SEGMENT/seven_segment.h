#ifndef SEVEN_SEGMENT_H
#define SEVEN_SEGMENT_H
#include "main.h"
#include <stdint.h>
typedef enum { SEG_COMMON_CATHODE=0, SEG_COMMON_ANODE=1 } SEG_Polarity;
typedef struct { GPIO_TypeDef *port[8]; uint16_t pin[8]; GPIO_TypeDef **digit_port; uint16_t *digit_pin; uint8_t digits; SEG_Polarity polarity; uint8_t reverse_digits; } SEG_Handle;
void SEG_Init(SEG_Handle*);
void SEG_WriteDigit(SEG_Handle*,uint8_t position,int8_t value,uint8_t decimal_point);
void SEG_Clear(SEG_Handle*);
void SEG_DisplayNumber(SEG_Handle*,int32_t value);
#endif