#include "oled.h"
static HAL_StatusTypeDef tx(OLED_Handle*h,uint8_t control,const uint8_t*d,uint16_t n){uint8_t b[129];if(n>128)return HAL_ERROR;b[0]=control;for(uint16_t i=0;i<n;i++)b[i+1]=d[i];return HAL_I2C_Master_Transmit(h->i2c,h->address,b,n+1,100);}
static HAL_StatusTypeDef command(OLED_Handle*h,uint8_t c){return tx(h,0x00,&c,1);}
HAL_StatusTypeDef OLED_Init(OLED_Handle*h,I2C_HandleTypeDef*i,uint8_t a){if(!h||!i)return HAL_ERROR;h->i2c=i;h->address=(uint8_t)(a<=0x7F?a<<1:a);HAL_Delay(50);uint8_t seq[]={0xAE,0xD5,0x80,0xA8,0x3F,0xD3,0x00,0x40,0x8D,0x14,0x20,0x00,0xA1,0xC8,0xDA,0x12,0x81,0x7F,0xD9,0xF1,0xDB,0x40,0xA4,0xA6,0xAF};for(unsigned k=0;k<sizeof(seq);k++)if(command(h,seq[k])!=HAL_OK)return HAL_ERROR;OLED_Clear(h);return OLED_Update(h);}
void OLED_Clear(OLED_Handle*h){if(h)for(unsigned i=0;i<sizeof(h->buffer);i++)h->buffer[i]=0;}
void OLED_DrawPixel(OLED_Handle*h,uint8_t x,uint8_t y,uint8_t on){if(!h||x>=128||y>=64)return;uint16_t i=x+(y/8)*128;uint8_t m=1<<(y&7);if(on)h->buffer[i]|=m;else h->buffer[i]&=(uint8_t)~m;}
HAL_StatusTypeDef OLED_Update(OLED_Handle*h){if(!h)return HAL_ERROR;for(uint8_t page=0;page<8;page++){uint8_t c[]={0xB0|page,0x00,0x10};if(tx(h,0x00,c,3)!=HAL_OK)return HAL_ERROR;if(tx(h,0x40,&h->buffer[page*128],128)!=HAL_OK)return HAL_ERROR;}return HAL_OK;}
static const uint8_t font5x7[96][5]={{0}};
void OLED_DrawChar(OLED_Handle*h,uint8_t x,uint8_t y,char c){if(!h)return;if(c<32||c>127)c='?';const uint8_t*g=font5x7[(uint8_t)c-32];for(int col=0;col<5;col++)for(int row=0;row<7;row++)OLED_DrawPixel(h,x+col,y+row,(g[col]>>row)&1);}
void OLED_DrawString(OLED_Handle*h,uint8_t x,uint8_t y,const char*s){if(!s)return;while(*s&&x<123){OLED_DrawChar(h,x,y,*s++);x+=6;}}