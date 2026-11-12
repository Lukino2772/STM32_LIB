#include "lcd_i2c.h"
#define LCD_RS 0x01
#define LCD_EN 0x04
#define LCD_BL 0x08
static HAL_StatusTypeDef expander(LCD_I2C_Handle*l,uint8_t v){return HAL_I2C_Master_Transmit(l->hi2c,l->address,&v,1,100);}
static HAL_StatusTypeDef pulse(LCD_I2C_Handle*l,uint8_t v){HAL_StatusTypeDef s=expander(l,v|LCD_EN);HAL_Delay(1);if(s!=HAL_OK)return s;s=expander(l,v&~LCD_EN);HAL_Delay(1);return s;}
static HAL_StatusTypeDef nibble(LCD_I2C_Handle*l,uint8_t n,uint8_t rs){uint8_t v=(n&0xF0)|(rs?LCD_RS:0)|(l->backlight?LCD_BL:0);HAL_StatusTypeDef s=expander(l,v);return s==HAL_OK?pulse(l,v):s;}
static HAL_StatusTypeDef send(LCD_I2C_Handle*l,uint8_t v,uint8_t rs){HAL_StatusTypeDef s=nibble(l,v&0xF0,rs);return s==HAL_OK?nibble(l,(uint8_t)(v<<4),rs):s;}
HAL_StatusTypeDef LCD_I2C_Init(LCD_I2C_Handle*l,I2C_HandleTypeDef*i,uint8_t a,uint8_t c,uint8_t r){if(!l||!i||!c||!r||r>4)return HAL_ERROR;l->hi2c=i;l->address=(uint8_t)(a<=0x7F?a<<1:a);l->columns=c;l->rows=r;l->backlight=1;l->display_control=0x0C;l->display_mode=0x06;HAL_Delay(50);nibble(l,0x30,0);HAL_Delay(5);nibble(l,0x30,0);HAL_Delay(1);nibble(l,0x30,0);nibble(l,0x20,0);send(l,(uint8_t)(0x20|(r>1?0x08:0)),0);send(l,0x08,0);LCD_I2C_Clear(l);send(l,(uint8_t)(0x04|l->display_mode),0);send(l,(uint8_t)(0x08|l->display_control),0);return HAL_OK;}
HAL_StatusTypeDef LCD_I2C_Clear(LCD_I2C_Handle*l){HAL_StatusTypeDef s=send(l,0x01,0);HAL_Delay(2);return s;}
HAL_StatusTypeDef LCD_I2C_SetCursor(LCD_I2C_Handle*l,uint8_t c,uint8_t r){static const uint8_t off[]={0x00,0x40,0x14,0x54};if(!l||r>=l->rows||c>=l->columns)return HAL_ERROR;return send(l,(uint8_t)(0x80+off[r]+c),0);}
HAL_StatusTypeDef LCD_I2C_Print(LCD_I2C_Handle*l,const char*t){if(!l||!t)return HAL_ERROR;while(*t){HAL_StatusTypeDef s=send(l,(uint8_t)*t++,1);if(s!=HAL_OK)return s;}return HAL_OK;}
HAL_StatusTypeDef LCD_I2C_Backlight(LCD_I2C_Handle*l,uint8_t on){if(!l)return HAL_ERROR;l->backlight=!!on;return expander(l,l->backlight?LCD_BL:0);}