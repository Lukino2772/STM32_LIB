#include "scope.h"
#include "lcd.h"
#include "main.h"

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;
#include <math.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#define ADC_BUFFER_SIZE 256
#define SAMPLE_RATE     10000.0f

#define GRAPH_X  10
#define GRAPH_Y  48
#define GRAPH_W  220
#define GRAPH_H  132

#define C_BG       0x0841
#define C_PANEL    0x10A2
#define C_GRID     0x2945
#define C_TEXT     0xFFFF
#define C_MUTED    0x8410
#define C_GREEN    0x07E0
#define C_CYAN     0x07FF
#define C_YELLOW   0xFFE0
#define C_ORANGE   0xFD20
#define C_RED      0xF800

static uint16_t adc_buffer[ADC_BUFFER_SIZE];
static uint16_t samples[ADC_BUFFER_SIZE];

static uint16_t sample_min;
static uint16_t sample_max;

static float voltage_pp;
static float voltage_rms;
static float frequency;

static volatile uint8_t capture_done = 0;

/* 5x7 bitmap font */
static const char font_chars[] =
" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.:/-+%";

static const uint8_t font[][5] =
{
    {0x00,0x00,0x00,0x00,0x00},
    {0x7E,0x11,0x11,0x11,0x7E},
    {0x7F,0x49,0x49,0x49,0x36},
    {0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C},
    {0x7F,0x49,0x49,0x49,0x41},
    {0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A},
    {0x7F,0x08,0x08,0x08,0x7F},
    {0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01},
    {0x7F,0x08,0x14,0x22,0x41},
    {0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F},
    {0x7F,0x04,0x08,0x10,0x7F},
    {0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06},
    {0x3E,0x41,0x51,0x21,0x5E},
    {0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},
    {0x01,0x01,0x7F,0x01,0x01},
    {0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F},
    {0x3F,0x40,0x38,0x40,0x3F},
    {0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07},
    {0x61,0x51,0x49,0x45,0x43},

    {0x3E,0x51,0x49,0x45,0x3E},
    {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46},
    {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10},
    {0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30},
    {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},
    {0x06,0x49,0x49,0x29,0x1E},

    {0x00,0x60,0x60,0x00,0x00},
    {0x00,0x36,0x36,0x00,0x00},
    {0x20,0x10,0x08,0x04,0x02},
    {0x08,0x08,0x08,0x08,0x08},
    {0x08,0x08,0x3E,0x08,0x08},
    {0x63,0x13,0x08,0x64,0x63}
};

static void DrawChar(uint16_t x, uint16_t y,
                     char ch, uint16_t color, uint8_t scale)
{
    ch = (char)toupper((unsigned char)ch);

    const char *p = strchr(font_chars, ch);

    if (p == NULL)
        return;

    uint8_t index = (uint8_t)(p - font_chars);

    for (uint8_t col = 0; col < 5; col++)
    {
        for (uint8_t row = 0; row < 7; row++)
        {
            if (font[index][col] & (1U << row))
            {
                LCD_FillRectangle(
                    x + col * scale,
                    y + row * scale,
                    scale,
                    scale,
                    color
                );
            }
        }
    }
}

static void DrawText(uint16_t x, uint16_t y,
                     const char *str,
                     uint16_t color,
                     uint8_t scale)
{
    while (*str)
    {
        DrawChar(x, y, *str, color, scale);
        x += 6 * scale;
        str++;
    }
}

static void DrawLine(int x0, int y0, int x1, int y1,
                     uint16_t color)
{
    int dx = (x1 > x0) ? x1 - x0 : x0 - x1;
    int sx = (x0 < x1) ? 1 : -1;

    int dy = (y1 > y0) ? y0 - y1 : y1 - y0;
    int sy = (y0 < y1) ? 1 : -1;

    int error = dx + dy;

    while (1)
    {
        LCD_DrawPixel(x0, y0, color);

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * error;

        if (e2 >= dy)
        {
            error += dy;
            x0 += sx;
        }

        if (e2 <= dx)
        {
            error += dx;
            y0 += sy;
        }
    }
}

static void DrawGrid(void)
{
    LCD_FillRectangle(
        GRAPH_X, GRAPH_Y,
        GRAPH_W, GRAPH_H,
        C_BG
    );

    for (uint8_t i = 0; i <= 10; i++)
    {
        uint16_t x = GRAPH_X + i * (GRAPH_W - 1) / 10;

        for (uint16_t y = GRAPH_Y; y < GRAPH_Y + GRAPH_H; y += 4)
            LCD_DrawPixel(x, y, C_GRID);
    }

    for (uint8_t i = 0; i <= 6; i++)
    {
        uint16_t y = GRAPH_Y + i * (GRAPH_H - 1) / 6;

        for (uint16_t x = GRAPH_X; x < GRAPH_X + GRAPH_W; x += 4)
            LCD_DrawPixel(x, y, C_GRID);
    }

    /* Center reference line */
    for (uint16_t x = GRAPH_X; x < GRAPH_X + GRAPH_W; x += 3)
        LCD_DrawPixel(x, GRAPH_Y + GRAPH_H / 2, C_MUTED);
}

static void DrawInterface(void)
{
    LCD_FillScreen(C_BG);

    /* Instrument header */
    LCD_FillRectangle(0, 0, 240, 34, C_PANEL);
    LCD_FillRectangle(0, 0, 4, 34, C_CYAN);
    DrawText(12, 7, "SCOPE", C_TEXT, 2);
    DrawText(88, 12, "DIGITAL", C_MUTED, 1);

    /* Channel and acquisition badges */
    LCD_FillRectangle(174, 7, 56, 20, C_GREEN);
    DrawText(181, 13, "CH1", C_BG, 1);

    /* Waveform section */
    DrawText(10, 39, "WAVEFORM", C_CYAN, 1);
    DrawText(164, 39, "PA0 / ADC1", C_MUTED, 1);

    DrawGrid();

    /* Graph frame */
    DrawLine(GRAPH_X - 1, GRAPH_Y - 1,
             GRAPH_X + GRAPH_W, GRAPH_Y - 1, C_MUTED);
    DrawLine(GRAPH_X - 1, GRAPH_Y + GRAPH_H,
             GRAPH_X + GRAPH_W, GRAPH_Y + GRAPH_H, C_MUTED);
    DrawLine(GRAPH_X - 1, GRAPH_Y - 1,
             GRAPH_X - 1, GRAPH_Y + GRAPH_H, C_MUTED);
    DrawLine(GRAPH_X + GRAPH_W, GRAPH_Y - 1,
             GRAPH_X + GRAPH_W, GRAPH_Y + GRAPH_H, C_MUTED);

    /* Measurement cards */
    LCD_FillRectangle(8, 188, 108, 48, C_PANEL);
    LCD_FillRectangle(124, 188, 108, 48, C_PANEL);
    LCD_FillRectangle(8, 242, 108, 48, C_PANEL);
    LCD_FillRectangle(124, 242, 108, 48, C_PANEL);

    /* Color-coded card accents */
    LCD_FillRectangle(8, 188, 3, 48, C_CYAN);
    LCD_FillRectangle(124, 188, 3, 48, C_YELLOW);
    LCD_FillRectangle(8, 242, 3, 48, C_GREEN);
    LCD_FillRectangle(124, 242, 3, 48, C_ORANGE);

    DrawText(16, 194, "FREQUENCY", C_CYAN, 1);
    DrawText(132, 194, "VPP", C_YELLOW, 1);
    DrawText(16, 248, "AC RMS", C_GREEN, 1);
    DrawText(132, 248, "PERIOD", C_ORANGE, 1);

    /* Footer: 220 displayed samples at 10 kS/s = 22 ms span */
    LCD_FillRectangle(0, 298, 240, 22, C_PANEL);
    LCD_FillRectangle(0, 298, 240, 1, C_GRID);
    DrawText(9, 305, "22MS SPAN", C_TEXT, 1);
    DrawText(105, 305, "10K S/S", C_CYAN, 1);
    DrawText(190, 305, "AUTO", C_GREEN, 1);
}

static void CaptureSamples(void)
{
    capture_done = 0;

    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buffer,
                          ADC_BUFFER_SIZE) != HAL_OK)
        return;

    if (HAL_TIM_Base_Start(&htim2) != HAL_OK)
    {
        HAL_ADC_Stop_DMA(&hadc1);
        return;
    }

    uint32_t start_tick = HAL_GetTick();
    while (!capture_done && (HAL_GetTick() - start_tick) < 100U)
    {
        /* DMA completion interrupt sets capture_done. */
    }

    HAL_TIM_Base_Stop(&htim2);
    HAL_ADC_Stop_DMA(&hadc1);

    if (!capture_done)
        return;

    for (uint16_t i = 0; i < ADC_BUFFER_SIZE; i++)
        samples[i] = adc_buffer[i];
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
        capture_done = 1;
}

static void CalculateMeasurements(void)
{
    uint32_t sum = 0;
    float square_sum = 0.0f;

    sample_min = 4095;
    sample_max = 0;

    for (uint16_t i = 0; i < GRAPH_W; i++)
    {
        uint16_t value = samples[i];

        if (value < sample_min)
            sample_min = value;

        if (value > sample_max)
            sample_max = value;

        sum += value;
    }

    float average = (float)sum / GRAPH_W;

    for (uint16_t i = 0; i < GRAPH_W; i++)
    {
        float difference = samples[i] - average;
        square_sum += difference * difference;
    }

    voltage_pp =
        ((float)(sample_max - sample_min) * 3.3f) / 4095.0f;

    voltage_rms =
        (sqrtf(square_sum / GRAPH_W) * 3.3f) / 4095.0f;

    /* Frequency measurement using rising crossings */
    uint32_t first_crossing = 0;
    uint32_t last_crossing = 0;
    uint32_t crossing_count = 0;

    uint16_t threshold =
        sample_min + (sample_max - sample_min) / 2;

    for (uint16_t i = 1; i < GRAPH_W; i++)
    {
        if (samples[i - 1] < threshold &&
            samples[i] >= threshold)
        {
            if (crossing_count == 0)
                first_crossing = i;

            last_crossing = i;
            crossing_count++;
        }
    }

    if (crossing_count >= 2)
    {
        float average_period =
            (float)(last_crossing - first_crossing) /
            (float)(crossing_count - 1);

        frequency = SAMPLE_RATE / average_period;
    }
    else
    {
        frequency = 0.0f;
    }
}

static void DrawWaveform(void)
{
    DrawGrid();

    uint16_t range = sample_max - sample_min;

    if (range < 10)
        range = 10;

    uint16_t margin = range / 10;

    int32_t low = (int32_t)sample_min - margin;
    int32_t high = (int32_t)sample_max + margin;

    if (low < 0)
        low = 0;

    if (high > 4095)
        high = 4095;

    uint32_t display_range = high - low;

    if (display_range == 0)
        display_range = 1;

    for (uint16_t i = 1; i < GRAPH_W; i++)
    {
        int y1 = GRAPH_Y + GRAPH_H - 1 -
            ((samples[i - 1] - low) * (GRAPH_H - 1)
             / display_range);

        int y2 = GRAPH_Y + GRAPH_H - 1 -
            ((samples[i] - low) * (GRAPH_H - 1)
             / display_range);

        DrawLine(
            GRAPH_X + i - 1, y1,
            GRAPH_X + i, y2,
            C_GREEN
        );
    }
}

static void DrawMeasurements(void)
{
    char text[24];
    uint32_t freq_hz = (uint32_t)(frequency + 0.5f);
    uint32_t vpp_centi = (uint32_t)(voltage_pp * 100.0f + 0.5f);
    uint32_t rms_centi = (uint32_t)(voltage_rms * 100.0f + 0.5f);

    /* Use integer formatting: works with newlib-nano even when
       floating-point printf support is not enabled in CubeIDE. */
    snprintf(text, sizeof(text), "%lu HZ", (unsigned long)freq_hz);
    LCD_FillRectangle(14, 207, 96, 22, C_PANEL);
    DrawText(15, 210, text, C_TEXT, 1);

    snprintf(text, sizeof(text), "%lu.%02lu V",
             (unsigned long)(vpp_centi / 100U),
             (unsigned long)(vpp_centi % 100U));
    LCD_FillRectangle(130, 207, 96, 22, C_PANEL);
    DrawText(131, 210, text, C_TEXT, 1);

    snprintf(text, sizeof(text), "%lu.%02lu V",
             (unsigned long)(rms_centi / 100U),
             (unsigned long)(rms_centi % 100U));
    LCD_FillRectangle(14, 261, 96, 22, C_PANEL);
    DrawText(15, 264, text, C_TEXT, 1);

    if (frequency > 0.1f)
    {
        uint32_t period_centi_ms =
            (uint32_t)(100000.0f / frequency + 0.5f);
        snprintf(text, sizeof(text), "%lu.%02lu MS",
                 (unsigned long)(period_centi_ms / 100U),
                 (unsigned long)(period_centi_ms % 100U));
    }
    else
    {
        snprintf(text, sizeof(text), "--- MS");
    }

    LCD_FillRectangle(130, 261, 96, 22, C_PANEL);
    DrawText(131, 264, text, C_TEXT, 1);
}

void Scope_Init(void)
{
    memset(adc_buffer, 0, sizeof(adc_buffer));
    memset(samples, 0, sizeof(samples));

    DrawInterface();
}

void Scope_Update(void)
{
    CaptureSamples();

    if (!capture_done)
        return;

    CalculateMeasurements();
    DrawWaveform();
    DrawMeasurements();
}
