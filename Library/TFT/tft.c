#include "tft.h"
static void cmd(TFT_Handle*h,uint8_t c){if(h->set_dc)h->set_dc(0,h->user);if(h->write_command)h->write_command(c,h->user);else if(h->spi)HAL_SPI_Transmit(h->spi,&c,1,100);if(h->set_dc)h->set_dc(1,h->user);}
static void data8(TFT_Handle*h,uint8_t d){if(h->write_data)h->write_data(d,8,h->user);else if(h->spi)HAL_SPI_Transmit(h->spi,&d,1,100);}
static void data16(TFT_Handle*h,uint16_t d){if(h->write_data)h->write_data(d,16,h->user);else{data8(h,d>>8);data8(h,d&255);}}
void TFT_WriteCommand(TFT_Handle*h,uint8_t c){if(h){if(h->set_cs)h->set_cs(1,h->user);cmd(h,c);if(h->set_cs)h->set_cs(0,h->user);}}
void TFT_WriteData(TFT_Handle*h,uint8_t d){if(h){if(h->set_cs)h->set_cs(1,h->user);data8(h,d);if(h->set_cs)h->set_cs(0,h->user);}}
void TFT_SetWindow(TFT_Handle*h,uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1){if(!h)return;if(h->set_cs)h->set_cs(1,h->user);if(h->type==TFT_BUS_PARALLEL8&&h->width==240){cmd(h,0x2A);data8(h,x0>>8);data8(h,x0);data8(h,x1>>8);data8(h,x1);cmd(h,0x2B);data8(h,y0>>8);data8(h,y0);data8(h,y1>>8);data8(h,y1);cmd(h,0x2C);}else{cmd(h,0x2A);data16(h,x0);data16(h,x1);cmd(h,0x2B);data16(h,y0);data16(h,y1);cmd(h,0x2C);}}
void TFT_DrawPixel(TFT_Handle*h,uint16_t x,uint16_t y,uint16_t c){if(!h||x>=h->width||y>=h->height)return;TFT_SetWindow(h,x,y,x,y);data16(h,c);if(h->set_cs)h->set_cs(0,h->user);}
void TFT_FillRect(TFT_Handle*h,uint16_t x,uint16_t y,uint16_t w,uint16_t ht,uint16_t c){if(!h||!w||!ht||x>=h->width||y>=h->height)return;if(x+w>h->width)w=h->width-x;if(y+ht>h->height)ht=h->height-y;TFT_SetWindow(h,x,y,x+w-1,y+ht-1);for(uint32_t i=0;i<(uint32_t)w*ht;i++)data16(h,c);if(h->set_cs)h->set_cs(0,h->user);}
void TFT_FillScreen(TFT_Handle*h,uint16_t c){if(h)TFT_FillRect(h,0,0,h->width,h->height,c);}
HAL_StatusTypeDef TFT_Init(TFT_Handle*h,TFT_Controller d){if(!h)return HAL_ERROR;if(h->set_reset){h->set_reset(0,h->user);HAL_Delay(20);h->set_reset(1,h->user);HAL_Delay(120);}if(h->set_cs)h->set_cs(1,h->user);switch(d){
case TFT_ILI9341: h->width=240;h->height=320;cmd(h,0x01);HAL_Delay(5);cmd(h,0x28);cmd(h,0x3A);data8(h,0x55);cmd(h,0x36);data8(h,h->madctl);cmd(h,0x11);HAL_Delay(120);cmd(h,0x29);break;
case TFT_ST7789: h->width=240;h->height=320;cmd(h,0x01);HAL_Delay(150);cmd(h,0x3A);data8(h,0x55);cmd(h,0x36);data8(h,h->madctl);cmd(h,0x11);HAL_Delay(120);cmd(h,0x29);break;
case TFT_ST7735: h->width=128;h->height=160;cmd(h,0x01);HAL_Delay(150);cmd(h,0x11);HAL_Delay(120);cmd(h,0x3A);data8(h,0x05);cmd(h,0x36);data8(h,h->madctl);cmd(h,0x29);break;
case TFT_ILI9488: h->width=320;h->height=480;cmd(h,0x01);HAL_Delay(10);cmd(h,0x3A);data8(h,0x55);cmd(h,0x36);data8(h,h->madctl);cmd(h,0x11);HAL_Delay(120);cmd(h,0x29);break;
case TFT_ILI9325: h->width=240;h->height=320;cmd(h,0x00);data8(h,0x00);cmd(h,0x03);data8(h,0x10);cmd(h,0x07);data8(h,0x33);break;
default: return HAL_ERROR;}if(h->set_cs)h->set_cs(0,h->user);return HAL_OK;}