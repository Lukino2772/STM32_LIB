#include "lcd.h"

/* Control pins */
#define LCD_RD_PORT  GPIOA
#define LCD_RD_PIN   GPIO_PIN_1

#define LCD_WR_PORT  GPIOA
#define LCD_WR_PIN   GPIO_PIN_2

#define LCD_RS_PORT  GPIOA
#define LCD_RS_PIN   GPIO_PIN_3

#define LCD_CS_PORT  GPIOA
#define LCD_CS_PIN   GPIO_PIN_4

#define LCD_RST_PORT GPIOA
#define LCD_RST_PIN  GPIO_PIN_5

/* Data bus:
   D0 = PB0
   D1 = PB1
   D2 = PB2
   D3 = PB5
   D4 = PB8
   D5 = PB9
   D6 = PB10
   D7 = PB12
*/

#define LCD_DATA_MASK (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_5 | \
                       GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_12)

/* Put all 8 data bits on the scattered pins with ONE atomic BSRR write
   (low half = set, high half = reset) instead of 8 HAL calls. */
static inline void LCD_WriteBus(uint8_t data)
{
    uint32_t set = 0;

    if (data & 0x01) set |= GPIO_PIN_0;
    if (data & 0x02) set |= GPIO_PIN_1;
    if (data & 0x04) set |= GPIO_PIN_2;
    if (data & 0x08) set |= GPIO_PIN_5;
    if (data & 0x10) set |= GPIO_PIN_8;
    if (data & 0x20) set |= GPIO_PIN_9;
    if (data & 0x40) set |= GPIO_PIN_10;
    if (data & 0x80) set |= GPIO_PIN_12;

    GPIOB->BSRR = set | ((LCD_DATA_MASK & ~set) << 16);
}

static inline void LCD_Write8(uint8_t data)
{
    LCD_WriteBus(data);

    LCD_WR_PORT->BSRR = (uint32_t)LCD_WR_PIN << 16;   /* WR low  */
    __NOP();
    __NOP();
    __NOP();
    LCD_WR_PORT->BSRR = LCD_WR_PIN;                   /* WR high (data latched) */
}

static void LCD_Command(uint8_t command)
{
    HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, GPIO_PIN_RESET);
    LCD_Write8(command);
}

static void LCD_Data(uint8_t data)
{
    HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, GPIO_PIN_SET);
    LCD_Write8(data);
}

static void LCD_Data16(uint16_t data)
{
    LCD_Data(data >> 8);
    LCD_Data(data & 0xFF);
}

/* ILI9325/ILI9328 over an 8-bit bus: register index and value are each
   sent as two bytes (high, low). */
static void LCD_Index16(uint16_t reg)
{
    LCD_Command(reg >> 8);
    LCD_Command(reg & 0xFF);
}

static void LCD_WriteReg16(uint16_t reg, uint16_t value)
{
    LCD_Index16(reg);
    LCD_Data16(value);
}

/* ---- Reading (used only to identify the controller) ---- */
static void LCD_BusMode(uint32_t mode)
{
    GPIO_InitTypeDef g = {0};
    g.Pin = LCD_DATA_MASK;
    g.Mode = mode;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &g);
}

static uint8_t LCD_Read8(void)
{
    LCD_RD_PORT->BSRR = (uint32_t)LCD_RD_PIN << 16;   /* RD low */
    for (volatile int i = 0; i < 40; i++) { __NOP(); }
    uint32_t in = GPIOB->IDR;
    LCD_RD_PORT->BSRR = LCD_RD_PIN;                   /* RD high */

    uint8_t v = 0;
    if (in & GPIO_PIN_0)  v |= 0x01;
    if (in & GPIO_PIN_1)  v |= 0x02;
    if (in & GPIO_PIN_2)  v |= 0x04;
    if (in & GPIO_PIN_5)  v |= 0x08;
    if (in & GPIO_PIN_8)  v |= 0x10;
    if (in & GPIO_PIN_9)  v |= 0x20;
    if (in & GPIO_PIN_10) v |= 0x40;
    if (in & GPIO_PIN_12) v |= 0x80;
    return v;
}

/* Visible in the debugger (Live Expressions): 0x9325 / 0x9328 / 0x9341,
   anything else (0x0000 / 0xFFFF) means the bus or wiring is not talking. */
uint16_t lcd_id = 0;
static uint8_t lcd_is9325 = 1;

static uint16_t LCD_ReadID(void)
{
    /* ILI9325/9328: register 0x0000 reads back the device code */
    LCD_Index16(0x0000);
    HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, GPIO_PIN_SET);
    LCD_BusMode(GPIO_MODE_INPUT);
    uint8_t hi = LCD_Read8();
    uint8_t lo = LCD_Read8();
    LCD_BusMode(GPIO_MODE_OUTPUT_PP);
    uint16_t id = ((uint16_t)hi << 8) | lo;
    if (id == 0x9325 || id == 0x9328)
        return id;

    /* ILI9341: command 0xD3 returns dummy, 0x00, 0x93, 0x41 */
    LCD_Command(0xD3);
    HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, GPIO_PIN_SET);
    LCD_BusMode(GPIO_MODE_INPUT);
    (void)LCD_Read8();
    (void)LCD_Read8();
    uint8_t b2 = LCD_Read8();
    uint8_t b3 = LCD_Read8();
    LCD_BusMode(GPIO_MODE_OUTPUT_PP);
    if ((((uint16_t)b2 << 8) | b3) == 0x9341)
        return 0x9341;

    return id;
}

static void LCD_Select(void)
{
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
}

static void LCD_Deselect(void)
{
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

static void LCD_HardwareReset(void)
{
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);

    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(150);
}

static void LCD_SetWindow(uint16_t x1, uint16_t y1,
                          uint16_t x2, uint16_t y2)
{
    if (lcd_is9325)
    {
        LCD_WriteReg16(0x50, x1);    // Horizontal window start
        LCD_WriteReg16(0x51, x2);    // Horizontal window end
        LCD_WriteReg16(0x52, y1);    // Vertical window start
        LCD_WriteReg16(0x53, y2);    // Vertical window end
        LCD_WriteReg16(0x20, x1);    // GRAM X address
        LCD_WriteReg16(0x21, y1);    // GRAM Y address
        LCD_Index16(0x22);           // GRAM write; pixel data follows
        return;
    }

    LCD_Command(0x2A);       // Column address set
    LCD_Data16(x1);
    LCD_Data16(x2);

    LCD_Command(0x2B);       // Page address set
    LCD_Data16(y1);
    LCD_Data16(y2);

    LCD_Command(0x2C);       // Memory write
}

/* ILI9325 initialization (standard vendor sequence) */
#define DLY 0xFFFF
static const uint16_t ili9325_init[][2] = {
    {0x00, 0x0001}, {DLY, 50},
    {0x01, 0x0100}, {0x02, 0x0700},
    {0x03, 0x1030},                 /* entry mode: BGR, increment, portrait */
    {0x04, 0x0000}, {0x08, 0x0202}, {0x09, 0x0000}, {0x0A, 0x0000},
    {0x0C, 0x0000}, {0x0D, 0x0000}, {0x0F, 0x0000},
    {0x10, 0x0000}, {0x11, 0x0007}, {0x12, 0x0000}, {0x13, 0x0000},
    {DLY, 200},
    {0x10, 0x1690}, {0x11, 0x0227}, {DLY, 50},
    {0x12, 0x001A}, {DLY, 50},
    {0x13, 0x1800}, {0x29, 0x002A}, {DLY, 50},
    {0x30, 0x0000}, {0x31, 0x0000}, {0x32, 0x0000}, {0x35, 0x0206},
    {0x36, 0x0808}, {0x37, 0x0007}, {0x38, 0x0201}, {0x39, 0x0000},
    {0x3C, 0x0000}, {0x3D, 0x0000},
    {0x20, 0x0000}, {0x21, 0x0000},
    {0x50, 0x0000}, {0x51, 0x00EF}, {0x52, 0x0000}, {0x53, 0x013F},
    {0x60, 0xA700}, {0x61, 0x0003}, {0x6A, 0x0000},
    {0x90, 0x0010}, {0x92, 0x0000}, {0x93, 0x0003},
    {0x95, 0x1100}, {0x97, 0x0000}, {0x98, 0x0000},
    {0x07, 0x0133},                 /* display ON */
    {DLY, 50},
};

static void LCD_ControllerInit9325(void)
{
    for (uint32_t i = 0; i < sizeof(ili9325_init) / sizeof(ili9325_init[0]); i++)
    {
        if (ili9325_init[i][0] == DLY)
            HAL_Delay(ili9325_init[i][1]);
        else
            LCD_WriteReg16(ili9325_init[i][0], ili9325_init[i][1]);
    }
}

/* ILI9341 initialization */
static void LCD_ControllerInit(void)
{
    LCD_Command(0x01);       // Software reset
    HAL_Delay(120);

    LCD_Command(0x28);       // Display OFF

    LCD_Command(0xCF);
    LCD_Data(0x00);
    LCD_Data(0x83);
    LCD_Data(0x30);

    LCD_Command(0xED);
    LCD_Data(0x64);
    LCD_Data(0x03);
    LCD_Data(0x12);
    LCD_Data(0x81);

    LCD_Command(0xE8);
    LCD_Data(0x85);
    LCD_Data(0x01);
    LCD_Data(0x79);

    LCD_Command(0xCB);
    LCD_Data(0x39);
    LCD_Data(0x2C);
    LCD_Data(0x00);
    LCD_Data(0x34);
    LCD_Data(0x02);

    LCD_Command(0xF7);
    LCD_Data(0x20);

    LCD_Command(0xEA);
    LCD_Data(0x00);
    LCD_Data(0x00);

    LCD_Command(0xC0);       // Power control 1
    LCD_Data(0x26);

    LCD_Command(0xC1);       // Power control 2
    LCD_Data(0x11);

    LCD_Command(0xC5);       // VCOM control 1
    LCD_Data(0x35);
    LCD_Data(0x3E);

    LCD_Command(0xC7);       // VCOM control 2
    LCD_Data(0xBE);

    LCD_Command(0x36);       // Memory access control
    LCD_Data(0x48);

    LCD_Command(0x3A);       // Pixel format
    LCD_Data(0x55);           // RGB565, 16-bit

    LCD_Command(0xB1);       // Frame rate
    LCD_Data(0x00);
    LCD_Data(0x1B);

    LCD_Command(0xB6);       // Display function control
    LCD_Data(0x0A);
    LCD_Data(0x82);
    LCD_Data(0x27);

    LCD_Command(0xF2);
    LCD_Data(0x00);

    LCD_Command(0x26);
    LCD_Data(0x01);

    LCD_Command(0xE0);       // Positive gamma
    LCD_Data(0x0F);
    LCD_Data(0x31);
    LCD_Data(0x2B);
    LCD_Data(0x0C);
    LCD_Data(0x0E);
    LCD_Data(0x08);
    LCD_Data(0x4E);
    LCD_Data(0xF1);
    LCD_Data(0x37);
    LCD_Data(0x07);
    LCD_Data(0x10);
    LCD_Data(0x03);
    LCD_Data(0x0E);
    LCD_Data(0x09);
    LCD_Data(0x00);

    LCD_Command(0xE1);       // Negative gamma
    LCD_Data(0x00);
    LCD_Data(0x0E);
    LCD_Data(0x14);
    LCD_Data(0x03);
    LCD_Data(0x11);
    LCD_Data(0x07);
    LCD_Data(0x31);
    LCD_Data(0xC1);
    LCD_Data(0x48);
    LCD_Data(0x08);
    LCD_Data(0x0F);
    LCD_Data(0x0C);
    LCD_Data(0x31);
    LCD_Data(0x36);
    LCD_Data(0x0F);

    LCD_Command(0x11);       // Sleep OUT
    HAL_Delay(120);

    LCD_Command(0x29);       // Display ON
    HAL_Delay(20);
}

/* Configure all LCD pins */
static void LCD_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Data bus outputs */
    GPIO_InitStruct.Pin =
        GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 |
        GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_9 |
        GPIO_PIN_10 | GPIO_PIN_12;

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Control outputs */
    GPIO_InitStruct.Pin =
        LCD_RD_PIN | LCD_WR_PIN | LCD_RS_PIN |
        LCD_CS_PIN | LCD_RST_PIN;

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Initial states */
    HAL_GPIO_WritePin(GPIOA,
        LCD_RD_PIN | LCD_WR_PIN | LCD_RS_PIN |
        LCD_CS_PIN | LCD_RST_PIN,
        GPIO_PIN_SET);

    HAL_GPIO_WritePin(GPIOB,
        GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 |
        GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_9 |
        GPIO_PIN_10 | GPIO_PIN_12,
        GPIO_PIN_RESET);
}

void LCD_Init(void)
{
    LCD_GPIO_Init();

    LCD_HardwareReset();

    LCD_Select();
    lcd_id = LCD_ReadID();

    if (lcd_id == 0x9341)
    {
        lcd_is9325 = 0;
        LCD_ControllerInit();
    }
    else
    {
        /* 0x9325, 0x9328, or unknown: use the ILI9325 sequence */
        lcd_is9325 = 1;
        LCD_ControllerInit9325();
    }
    LCD_Deselect();

    HAL_Delay(50);
}

void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT)
        return;

    LCD_Select();

    LCD_SetWindow(x, y, x, y);
    LCD_Data16(color);

    LCD_Deselect();
}

void LCD_FillRectangle(uint16_t x, uint16_t y,
                       uint16_t width, uint16_t height,
                       uint16_t color)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT ||
        width == 0 || height == 0)
        return;

    if (width > LCD_WIDTH - x)
        width = LCD_WIDTH - x;

    if (height > LCD_HEIGHT - y)
        height = LCD_HEIGHT - y;

    LCD_Select();

    LCD_SetWindow(x, y,
                  x + width - 1,
                  y + height - 1);

    uint32_t pixels = (uint32_t)width * height;
    uint8_t hi = color >> 8, lo = color & 0xFF;

    HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, GPIO_PIN_SET);   /* data mode, once */
    for (uint32_t i = 0; i < pixels; i++)
    {
        LCD_Write8(hi);
        LCD_Write8(lo);
    }

    LCD_Deselect();
}

void LCD_FillScreen(uint16_t color)
{
    LCD_FillRectangle(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}
