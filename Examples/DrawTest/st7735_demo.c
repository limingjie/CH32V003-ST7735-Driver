/*
 * ST7735 Driver for CH32V003 - Demo
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

#include "ch32v003fun.h"
#include "st7735.h"

#include <stdint.h>

uint8_t rand8(void);

void popup(const char *msg, uint32_t delay)
{
    for (int i = 1; i < 11; i++)
    {
        tft_fill_rect(110 - (i << 2), 30 - (i << 1), i << 3, i << 2, BLACK);
        Delay_Ms(10);
    }
    tft_set_cursor(83, 26);

    tft_print(msg);

    Delay_Ms(delay);
}

int main(void)
{
    SystemInit();
    Delay_Ms(100);
    tft_init();

    uint32_t frame = 0;

    uint16_t colors[] = {
        BLACK, NAVY, DARKGREEN, DARKCYAN, MAROON, PURPLE, OLIVE,  LIGHTGREY,   DARKGREY, BLUE,
        GREEN, CYAN, RED,       MAGENTA,  YELLOW, WHITE,  ORANGE, GREENYELLOW, PINK,
    };

    tft_fill_rect(0, 0, 160, 80, BLACK);

    tft_set_color(RED);
    tft_set_background_color(BLACK);

    while (1)
    {
        popup("Draw Point", 1000);
        tft_fill_rect(0, 0, 160, 80, BLACK);

        frame = 30000;
        while (frame-- > 0)
        {
            tft_draw_pixel(rand8() % 160, rand8() % 80, colors[rand8() % 19]);
        }

        popup("Scan Line", 1000);
        tft_fill_rect(0, 0, 160, 80, BLACK);

        frame = 50;
        while (frame-- > 0)
        {
            for (uint8_t i = 0; i < 160; i++)
            {
                tft_draw_line(i, 0, i, 80, colors[rand8() % 19]);
            }
        }
        frame = 50;
        while (frame-- > 0)
        {
            for (uint8_t i = 0; i < 80; i++)
            {
                tft_draw_line(0, i, 180, i, colors[rand8() % 19]);
            }
        }

        popup("Draw Line", 1000);
        tft_fill_rect(0, 0, 160, 80, BLACK);

        frame = 2000;
        while (frame-- > 0)
        {
            tft_draw_line(rand8() % 160, rand8() % 80, rand8() % 160, rand8() % 80, colors[rand8() % 19]);
        }

        popup("Scan Rect", 1000);
        tft_fill_rect(0, 0, 160, 80, BLACK);

        frame = 100;
        while (frame-- > 0)
        {
            for (uint8_t i = 0; i < 40; i++)
            {
                tft_draw_rect(i, i, 160 - (i << 1), 80 - (i << 1), colors[rand8() % 19]);
            }
        }

        popup("Draw Rect", 1000);
        tft_fill_rect(0, 0, 160, 80, BLACK);

        frame = 5000;
        while (frame-- > 0)
        {
            tft_draw_rect(rand8() % 140, rand8() % 60, 20, 20, colors[rand8() % 19]);
        }

        popup("Fill Rect", 1000);
        tft_fill_rect(0, 0, 160, 80, BLACK);

        frame = 5000;
        while (frame-- > 0)
        {
            tft_fill_rect(rand8() % 140, rand8() % 60, 20, 20, colors[rand8() % 19]);
        }
    }
}

/* White Noise Generator State */
#define NOISE_BITS      8
#define NOISE_MASK      ((1 << NOISE_BITS) - 1)
#define NOISE_POLY_TAP0 31
#define NOISE_POLY_TAP1 21
#define NOISE_POLY_TAP2 1
#define NOISE_POLY_TAP3 0
uint32_t lfsr = 1;

/*
 * random byte generator
 */
uint8_t rand8(void)
{
    uint8_t  bit;
    uint32_t new_data;

    for (bit = 0; bit < NOISE_BITS; bit++)
    {
        new_data = ((lfsr >> NOISE_POLY_TAP0) ^ (lfsr >> NOISE_POLY_TAP1) ^ (lfsr >> NOISE_POLY_TAP2) ^
                    (lfsr >> NOISE_POLY_TAP3));
        lfsr     = (lfsr << 1) | (new_data & 1);
    }

    return lfsr & NOISE_MASK;
}
