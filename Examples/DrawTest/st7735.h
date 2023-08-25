/*
 * ST7735 Driver for CH32V003
 *
 * Reference:
 *  - https://github.com/moononournation/Arduino_GFX
 *  - https://gitee.com/morita/ch32-v003/tree/master/Driver
 *  - https://github.com/cnlohr/ch32v003fun/tree/master/examples/spi_oled
 *
 * Aug 2023 by Li Mingjie
 *  - Email:  limingjie@outlook.com
 *  - GitHub: https://github.com/limingjie/
 */

#ifndef __ST7735_H__
#define __ST7735_H__

#include <stdint.h>

// Define screen resolution and offset
#define ST7735_WIDTH    160
#define ST7735_HEIGHT   80
#define ST7735_X_OFFSET 1
#define ST7735_Y_OFFSET 26

// Wiring
// | CH32V003       | ST7735    | Power | Description                       |
// | -------------- | --------- | ----- | --------------------------------- |
// |                | 1 - LEDA  | 3V3   | Use PWM to control brightness     |
// |                | 2 - GND   | GND   | GND                               |
// | PC2            | 3 - RESET |       | Reset                             |
// | PC3            | 4 - RS    |       | DC (Data / Command)               |
// | PC6 (SPI MOSI) | 5 - SDA   |       | SPI MOSI (Master Output Slave In) |
// | PC5 (SPI SCLK) | 6 - SCL   |       | SPI SCLK (Serial Clock)           |
// |                | 7 - VDD   | 3V3   | VDD                               |
// | PC4            | 8 - CS    |       | SPI CS/SS (Chip/Slave Select)     |

// Note: To not use CS, uncomment the following line. Make ST7735 always enabled by pulling CS to ground.
//  #define ST7735_NO_CS

#define RGB565(r, g, b) ((((r)&0xF8) << 8) | (((g)&0xFC) << 3) | ((b) >> 3))
#define BGR565(r, g, b) ((((b)&0xF8) << 8) | (((g)&0xFC) << 3) | ((r) >> 3))
#define RGB             RGB565

#define BLACK       RGB(0, 0, 0)
#define NAVY        RGB(0, 0, 123)
#define DARKGREEN   RGB(0, 125, 0)
#define DARKCYAN    RGB(0, 125, 123)
#define MAROON      RGB(123, 0, 0)
#define PURPLE      RGB(123, 0, 123)
#define OLIVE       RGB(123, 125, 0)
#define LIGHTGREY   RGB(198, 195, 198)
#define DARKGREY    RGB(123, 125, 123)
#define BLUE        RGB(0, 0, 255)
#define GREEN       RGB(0, 255, 0)
#define CYAN        RGB(0, 255, 255)
#define RED         RGB(255, 0, 0)
#define MAGENTA     RGB(255, 0, 255)
#define YELLOW      RGB(255, 255, 0)
#define WHITE       RGB(255, 255, 255)
#define ORANGE      RGB(255, 165, 0)
#define GREENYELLOW RGB(173, 255, 41)
#define PINK        RGB(255, 130, 198)

void tft_init(void);

// Text functions
void tft_set_cursor(uint8_t x, uint8_t y);
void tft_set_color(uint16_t color);
void tft_set_background_color(uint16_t color);
void tft_print_char(char c);
void tft_print(const char* str);
void tft_print_number(int32_t num);

// Drawing functions
void tft_draw_pixel(uint8_t x, uint8_t y, uint16_t color);
void tft_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void tft_draw_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint16_t color);
void tft_fill_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint16_t color);
void tft_draw_bitmap(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t* bitmap);

#endif  // __ST7735_H__
