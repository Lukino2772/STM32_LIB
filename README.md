# STM32 Peripheral Library

STM32 HAL-based reusable drivers, initially targeted at STM32F4 / STM32F411CEU6. Add the relevant `Library/**` folders to your STM32CubeIDE include and source paths.

## Modules
- `LCD_I2C`: HD44780 character LCD with PCF8574 I2C backpack (16x2 / 20x4).
- `ACCEL_GYRO`: MPU6050, MPU6500 and MPU9250-family accelerometer/gyroscope over I2C.
- `SEVEN_SEGMENT`: GPIO-driven single-digit output and simple multiplexed display helper.
- `TFT`: common graphics primitives and selectable ILI9325, ILI9341, ILI9488, ST7735 and ST7789 controller entry points. Bus transport is supplied by callbacks for SPI, 8-bit parallel or 16-bit parallel wiring.
- `NEOPIXEL`: WS2812/NeoPixel PWM + DMA transport.
- `OLED`: SSD1306 128x64 I2C framebuffer driver.

## Important integration notes
1. These are HAL drivers. Include the module header from your application and pass the CubeMX-generated peripheral handles.
2. Configure GPIO, I2C, SPI and timers in CubeMX. No driver assumes fixed Black Pill pin assignments.
3. For TFT, implement the transport callbacks in `TFT_Handle`. For SPI, set `spi`; for parallel buses, implement `write_command` and `write_data` callbacks and control CS/DC/reset as required. Controller init sequences may need module-specific power/gamma and panel-offset tuning.
4. NeoPixel requires a timer PWM channel configured for a 800 kHz bit period and a DMA buffer of at least `24 * led_count + reset_slots` halfwords. Call `NeoPixel_Show` after updating pixels; stop/re-arm DMA in the HAL completion callback before the next frame.
5. The SSD1306 font table is a placeholder in this initial commit; framebuffer and pixel/update functions are ready, but text glyphs must be populated with a font table before text rendering.
6. MPU devices use 7-bit I2C addresses by default (e.g. 0x68); the driver shifts them for STM32 HAL.
7. The seven-segment helper assumes GPIO pins and digit-enable polarity are wired/configured by the application. Multiplexing should be called periodically from a timer/task for flicker-free multi-digit output.

## TFT buses
All three bus types are represented in the handle: SPI via HAL SPI, and 8/16-bit parallel via user callbacks. Wire-specific timing, WR/RD strobes and data GPIO mapping belong in those callbacks. Do not connect a 5 V parallel module directly to STM32 pins unless its logic levels are compatible.
