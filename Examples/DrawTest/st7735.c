/// \brief ST7735 Driver for CH32V003
///
/// \author Li Mingjie
///  - Email:  limingjie@outlook.com
///  - GitHub: https://github.com/limingjie/
///
/// \date Aug 2023
///
/// \section References
///  - https://github.com/moononournation/Arduino_GFX
///  - https://gitee.com/morita/ch32-v003/tree/master/Driver
///  - https://github.com/cnlohr/ch32v003fun/tree/master/examples/spi_oled
///
/// \copyright Attribution-NonCommercial-ShareAlike 4.0 (CC BY-NC-SA 4.0)

#include "st7735.h"

#ifdef PLATFORMIO  // Use PlatformIO CH32V
    #include <debug.h>
#else  // Use ch32v003fun
    #include "ch32v003fun.h"
#endif

#include "font5x7.h"

// CH32V003 Pin Definitions
#define PIN_RESET 2  // PC2
#define PIN_DC    3  // PC3
#ifndef ST7735_NO_CS
    #define PIN_CS 4  // PC4
#endif
#define SPI_SCLK 5  // PC5
#define SPI_MOSI 6  // PC6

#define DATA_MODE()    (GPIOC->BSHR |= 1 << PIN_DC)  // DC High
#define COMMAND_MODE() (GPIOC->BCR |= 1 << PIN_DC)   // DC Low
#define RESET_HIGH()   (GPIOC->BSHR |= 1 << PIN_RESET)
#define RESET_LOW()    (GPIOC->BCR |= 1 << PIN_RESET)
#ifndef ST7735_NO_CS
    #define START_WRITE() (GPIOC->BCR |= 1 << PIN_CS)   // CS Low
    #define END_WRITE()   (GPIOC->BSHR |= 1 << PIN_CS)  // CS High
#else
    #define START_WRITE()
    #define END_WRITE()
#endif

// PlatformIO Compatibility
#ifdef PLATFORMIO
    #define CTLR1_SPE_Set      ((uint16_t)0x0040)
    #define GPIO_CNF_OUT_PP    0x00
    #define GPIO_CNF_OUT_PP_AF 0x08
#endif

// ST7735 Datasheet
// https://www.displayfuture.com/Display/datasheet/controller/ST7735.pdf
// Delays
#define ST7735_RST_DELAY    50   // delay ms wait for reset finish
#define ST7735_SLPOUT_DELAY 120  // delay ms wait for sleep out finish

// System Function Command List - Write Commands Only
#define ST7735_SLPIN   0x10  // Sleep IN
#define ST7735_SLPOUT  0x11  // Sleep Out
#define ST7735_PTLON   0x12  // Partial Display Mode On
#define ST7735_NORON   0x13  // Normal Display Mode On
#define ST7735_INVOFF  0x20  // Display Inversion Off
#define ST7735_INVON   0x21  // Display Inversion On
#define ST7735_GAMSET  0x26  // Gamma Set
#define ST7735_DISPOFF 0x28  // Display Off
#define ST7735_DISPON  0x29  // Display On
#define ST7735_CASET   0x2A  // Column Address Set
#define ST7735_RASET   0x2B  // Row Address Set
#define ST7735_RAMWR   0x2C  // Memory Write
#define ST7735_PLTAR   0x30  // Partial Area
#define ST7735_TEOFF   0x34  // Tearing Effect Line Off
#define ST7735_TEON    0x35  // Tearing Effect Line On
#define ST7735_MADCTL  0x36  // Memory Data Access Control
#define ST7735_IDMOFF  0x38  // Idle Mode Off
#define ST7735_IDMON   0x39  // Idle Mode On
#define ST7735_COLMOD  0x3A  // Interface Pixel Format

// Panel Function Command List - Only Used
#define ST7735_GMCTRP1 0xE0  // Gamma '+' polarity Correction Characteristics Setting
#define ST7735_GMCTRN1 0xE1  // Gamma '-' polarity Correction Characteristics Setting

// MADCTL Parameters
#define ST7735_MADCTL_MH  0x04  // Bit 2 - Refresh Left to Right
#define ST7735_MADCTL_RGB 0x00  // Bit 3 - RGB Order
#define ST7735_MADCTL_BGR 0x08  // Bit 3 - BGR Order
#define ST7735_MADCTL_ML  0x10  // Bit 4 - Scan Address Increase
#define ST7735_MADCTL_MV  0x20  // Bit 5 - X-Y Exchange
#define ST7735_MADCTL_MX  0x40  // Bit 6 - X-Mirror
#define ST7735_MADCTL_MY  0x80  // Bit 7 - Y-Mirror

// COLMOD Parameter
#define ST7735_COLMOD_16_BPP 0x05  // 101 - 16-bit/pixel

// 5x7 Font
#define FONT_WIDTH  5  // Font width
#define FONT_HEIGHT 7  // Font height

static uint16_t _cursor_x                  = 0;
static uint16_t _cursor_y                  = 0;      // Cursor position (x, y)
static uint16_t _color                     = WHITE;  // Color
static uint16_t _bg_color                  = BLACK;  // Background color
static uint8_t  _buffer[ST7735_WIDTH << 1] = {0};    // DMA buffer, long enough to fill a row.

/// \brief Initialize ST7735
/// \details Configure SPI, DMA, and RESET/DC/CS lines.
static void SPI_init(void)
{
    // Enable GPIO Port C and SPI peripheral
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_SPI1;

    // PC2 - RESET
    GPIOC->CFGLR &= ~(0xf << (PIN_RESET << 2));
    GPIOC->CFGLR |= (GPIO_CNF_OUT_PP | GPIO_Speed_50MHz) << (PIN_RESET << 2);

    // PC3 - DC
    GPIOC->CFGLR &= ~(0xf << (PIN_DC << 2));
    GPIOC->CFGLR |= (GPIO_CNF_OUT_PP | GPIO_Speed_50MHz) << (PIN_DC << 2);

#ifndef ST7735_NO_CS
    // PC4 - CS
    GPIOC->CFGLR &= ~(0xf << (PIN_CS << 2));
    GPIOC->CFGLR |= (GPIO_CNF_OUT_PP | GPIO_Speed_50MHz) << (PIN_CS << 2);
#endif

    // PC5 - SCLK
    GPIOC->CFGLR &= ~(0xf << (SPI_SCLK << 2));
    GPIOC->CFGLR |= (GPIO_CNF_OUT_PP_AF | GPIO_Speed_50MHz) << (SPI_SCLK << 2);

    // PC6 - MOSI
    GPIOC->CFGLR &= ~(0xf << (SPI_MOSI << 2));
    GPIOC->CFGLR |= (GPIO_CNF_OUT_PP_AF | GPIO_Speed_50MHz) << (SPI_MOSI << 2);

    // Configure SPI
    SPI1->CTLR1 = SPI_CPHA_1Edge             // Bit 0     - Clock PHAse
                  | SPI_CPOL_Low             // Bit 1     - Clock POLarity - idles at the logical low voltage
                  | SPI_Mode_Master          // Bit 2     - Master device
                  | SPI_BaudRatePrescaler_2  // Bit 3-5   - F_HCLK / 2
                  | SPI_FirstBit_MSB         // Bit 7     - MSB transmitted first
                  | SPI_NSS_Soft             // Bit 9     - Software slave management
                  | SPI_DataSize_8b          // Bit 11    - 8-bit data
                  | SPI_Direction_1Line_Tx;  // Bit 14-15 - 1-line SPI, transmission only
    SPI1->CRCR = 7;                          // CRC
    SPI1->CTLR2 |= SPI_I2S_DMAReq_Tx;        // Configure SPI DMA Transfer
    SPI1->CTLR1 |= CTLR1_SPE_Set;            // Bit 6     - Enable SPI

    // Enable DMA peripheral
    RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;

    // Config DMA for SPI TX
    DMA1_Channel3->CFGR = DMA_DIR_PeripheralDST          // Bit 4     - Read from memory
                          | DMA_Mode_Circular            // Bit 5     - Circulation mode
                          | DMA_PeripheralInc_Disable    // Bit 6     - Peripheral address no change
                          | DMA_MemoryInc_Enable         // Bit 7     - Increase memory address
                          | DMA_PeripheralDataSize_Byte  // Bit 8-9   - 8-bit data
                          | DMA_MemoryDataSize_Byte      // Bit 10-11 - 8-bit data
                          | DMA_Priority_VeryHigh        // Bit 12-13 - Very high priority
                          | DMA_M2M_Disable;             // Bit 14    - Disable memory to memory mode
    DMA1_Channel3->PADDR = (uint32_t)&SPI1->DATAR;
}

/// \brief Send Data Through SPI via DMA
/// \param buffer Memory address
/// \param size Memory size
/// \param repeat Repeat times
static void SPI_send_DMA(const uint8_t* buffer, uint16_t size, uint16_t repeat)
{
    DMA1_Channel3->MADDR = (uint32_t)buffer;
    DMA1_Channel3->CNTR  = size;
    DMA1_Channel3->CFGR |= DMA_CFGR1_EN;  // Turn on channel

    // Circulate the buffer
    while (repeat--)
    {
        // Clear flag, start sending?
        DMA1->INTFCR = DMA1_FLAG_TC3;

        // Waiting for channel 3 transmission complete
        while (!(DMA1->INTFR & DMA1_FLAG_TC3))
            ;
    }

    DMA1_Channel3->CFGR &= ~DMA_CFGR1_EN;  // Turn off channel
}

/// \brief Send Data Directly Through SPI
/// \param data 8-bit data
static void SPI_send(uint8_t data)
{
    // Send byte
    SPI1->DATAR = data;

    // Waiting for transmission complete
    while (!(SPI1->STATR & SPI_STATR_TXE))
        ;
}

/// \brief Send 8-Bit Command
/// \param cmd 8-bit command
static void write_command_8(uint8_t cmd)
{
    COMMAND_MODE();
    SPI_send(cmd);
}

/// \brief Send 8-Bit Data
/// \param cmd 8-bit data
static void write_data_8(uint8_t data)
{
    DATA_MODE();
    SPI_send(data);
}

/// \brief Send 16-Bit Data
/// \param cmd 16-bit data
static void write_data_16(uint16_t data)
{
    DATA_MODE();
    SPI_send(data >> 8);
    SPI_send(data);
}

/// \brief Initialize ST7735
/// \details Initialization sequence from Arduino_GFX
/// https://github.com/moononournation/Arduino_GFX/blob/master/src/display/Arduino_ST7735.h
void tft_init(void)
{
    SPI_init();

    // Reset display
    RESET_LOW();
    Delay_Ms(ST7735_RST_DELAY);
    RESET_HIGH();
    Delay_Ms(ST7735_RST_DELAY);

    START_WRITE();

    // Out of sleep mode, no args, w/delay
    write_command_8(ST7735_SLPOUT);
    Delay_Ms(ST7735_SLPOUT_DELAY);

    // Set rotation
    write_command_8(ST7735_MADCTL);
    write_data_8(ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_BGR);  // 0 - Horizontal
    // write_data_8(ST7735_MADCTL_BGR);                                        // 1 - Vertical
    // write_data_8(ST7735_MADCTL_MX | ST7735_MADCTL_MV | ST7735_MADCTL_BGR);  // 2 - Horizontal
    // write_data_8(ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_BGR);  // 3 - Vertical

    // Set Interface Pixel Format - 16-bit/pixel
    write_command_8(ST7735_COLMOD);
    write_data_8(ST7735_COLMOD_16_BPP);

    // Gamma Adjustments (pos. polarity), 16 args.
    // (Not entirely necessary, but provides accurate colors)
    uint8_t gamma_p[] = {0x09, 0x16, 0x09, 0x20, 0x21, 0x1B, 0x13, 0x19,
                         0x17, 0x15, 0x1E, 0x2B, 0x04, 0x05, 0x02, 0x0E};
    write_command_8(ST7735_GMCTRP1);
    DATA_MODE();
    SPI_send_DMA(gamma_p, 16, 1);

    // Gamma Adjustments (neg. polarity), 16 args.
    // (Not entirely necessary, but provides accurate colors)
    uint8_t gamma_n[] = {0x0B, 0x14, 0x08, 0x1E, 0x22, 0x1D, 0x18, 0x1E,
                         0x1B, 0x1A, 0x24, 0x2B, 0x06, 0x06, 0x02, 0x0F};
    write_command_8(ST7735_GMCTRN1);
    DATA_MODE();
    SPI_send_DMA(gamma_n, 16, 1);

    Delay_Ms(10);

    // Invert display
    write_command_8(ST7735_INVON);
    // write_command_8(ST7735_INVOFF);

    // Normal display on, no args, w/delay
    write_command_8(ST7735_NORON);
    Delay_Ms(10);

    // Main screen turn on, no args, w/delay
    write_command_8(ST7735_DISPON);
    Delay_Ms(10);

    END_WRITE();
}

/// \brief Set Cursor Position for Print Functions
/// \param x X coordinate, from left to right.
/// \param y Y coordinate, from top to bottom.
/// \details Calculate offset and set to `_cursor_x` and `_cursor_y` variables
void tft_set_cursor(uint16_t x, uint16_t y)
{
    _cursor_x = x + ST7735_X_OFFSET;
    _cursor_y = y + ST7735_Y_OFFSET;
}

/// \brief Set Text Color
/// \param color Text color
/// \details Set to `_color` variable
void tft_set_color(uint16_t color)
{
    _color = color;
}

/// \brief Set Text Background Color
/// \param color Text background color
/// \details Set to `_bg_color` variable
void tft_set_background_color(uint16_t color)
{
    _bg_color = color;
}

/// \brief Set Memory Write Window
/// \param x0 Start column
/// \param y0 Start row
/// \param x1 End column
/// \param y1 End row
static void tft_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    write_command_8(ST7735_CASET);
    write_data_16(x0);
    write_data_16(x1);
    write_command_8(ST7735_RASET);
    write_data_16(y0);
    write_data_16(y1);
    write_command_8(ST7735_RAMWR);
}

/// \brief Print a Character
/// \param c Character to print
/// \details DMA accelerated.
void tft_print_char(char c)
{
    const unsigned char* start = &font[c + (c << 2)];

    uint16_t sz = 0;
    for (uint8_t i = 0; i < FONT_HEIGHT; i++)
    {
        for (uint8_t j = 0; j < FONT_WIDTH; j++)
        {
            if ((*(start + j)) & (0x01 << i))
            {
                _buffer[sz++] = _color >> 8;
                _buffer[sz++] = _color;
            }
            else
            {
                _buffer[sz++] = _bg_color >> 8;
                _buffer[sz++] = _bg_color;
            }
        }
    }

    START_WRITE();
    tft_set_window(_cursor_x, _cursor_y, _cursor_x + FONT_WIDTH - 1, _cursor_y + FONT_HEIGHT - 1);
    DATA_MODE();
    SPI_send_DMA(_buffer, sz, 1);
    END_WRITE();
}

/// \brief Print a String
/// \param str String to print
void tft_print(const char* str)
{
    while (*str)
    {
        tft_print_char(*str++);
        _cursor_x += FONT_WIDTH + 1;
    }
}

/// \brief Print an Integer
/// \param num Number to print
/// \param width Expected width of the number.
/// Align left if it is less than the width of the number.
/// Align right if it is greater than the width of the number.
void tft_print_number(int32_t num, uint16_t width)
{
    static char str[12];
    uint8_t     position  = 11;
    uint8_t     negative  = 0;
    uint16_t    num_width = 0;

    // Handle negative number
    if (num < 0)
    {
        negative = 1;
        num      = -num;
    }

    str[position] = '\0';  // End of the string.
    while (num)
    {
        str[--position] = num % 10 + '0';
        num /= 10;
    }

    if (position == 11)
    {
        str[--position] = '0';
    }

    if (negative)
    {
        str[--position] = '-';
    }

    // Calculate alignment
    num_width = (11 - position) * (FONT_WIDTH + 1) - 1;
    if (width > num_width)
    {
        _cursor_x += width - num_width;
    }

    tft_print(&str[position]);
}

/// \brief Draw a Pixel
/// \param x X
/// \param y Y
/// \param color Pixel color
/// \details SPI direct write
void tft_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    x += ST7735_X_OFFSET;
    y += ST7735_Y_OFFSET;
    START_WRITE();
    tft_set_window(x, y, x, y);
    write_data_16(color);
    END_WRITE();
}

/// \brief Fill a Rectangle Area
/// \param x Start X coordinate
/// \param y Start Y coordinate
/// \param width Width
/// \param height Height
/// \param color Fill Color
/// \details DMA accelerated.
void tft_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    x += ST7735_X_OFFSET;
    y += ST7735_Y_OFFSET;

    uint16_t sz = 0;
    for (uint16_t x = 0; x < width; x++)
    {
        _buffer[sz++] = color >> 8;
        _buffer[sz++] = color;
    }

    START_WRITE();
    tft_set_window(x, y, x + width - 1, y + height - 1);
    DATA_MODE();
    SPI_send_DMA(_buffer, sz, height);
    END_WRITE();
}

/// \brief Draw a Bitmap
/// \param x Start X coordinate
/// \param y Start Y coordinate
/// \param width Width
/// \param height Height
/// \param bitmap Bitmap
void tft_draw_bitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t* bitmap)
{
    x += ST7735_X_OFFSET;
    y += ST7735_Y_OFFSET;
    START_WRITE();
    tft_set_window(x, y, x + width - 1, y + height - 1);
    DATA_MODE();
    SPI_send_DMA(bitmap, width * height << 1, 1);
    END_WRITE();
}

/// \brief Draw a Vertical Line Fast
/// \param x0 Start X coordinate
/// \param y0 Start Y coordinate
/// \param x1 End X coordinate
/// \param y1 End Y coordinate
/// \param color Line color
/// \details DMA accelerated
static void _tft_draw_fast_v_line(int16_t x, int16_t y, int16_t h, uint16_t color)
{
    x += ST7735_X_OFFSET;
    y += ST7735_Y_OFFSET;

    uint16_t sz = 0;
    for (int16_t j = 0; j < h; j++)
    {
        _buffer[sz++] = color >> 8;
        _buffer[sz++] = color;
    }

    START_WRITE();
    tft_set_window(x, y, x, y + h - 1);
    DATA_MODE();
    SPI_send_DMA(_buffer, sz, 1);
    END_WRITE();
}

/// \brief Draw a Horizontal Line Fast
/// \param x0 Start X coordinate
/// \param y0 Start Y coordinate
/// \param x1 End X coordinate
/// \param y1 End Y coordinate
/// \param color Line color
/// \details DMA accelerated
static void _tft_draw_fast_h_line(int16_t x, int16_t y, int16_t w, uint16_t color)
{
    x += ST7735_X_OFFSET;
    y += ST7735_Y_OFFSET;

    uint16_t sz = 0;
    for (int16_t j = 0; j < w; j++)
    {
        _buffer[sz++] = color >> 8;
        _buffer[sz++] = color;
    }

    START_WRITE();
    tft_set_window(x, y, x + w - 1, y);
    DATA_MODE();
    SPI_send_DMA(_buffer, sz, 1);
    END_WRITE();
}

// Draw line helpers
#define _diff(a, b) ((a > b) ? (a - b) : (b - a))
#define _swap_int16_t(a, b) \
    {                       \
        int16_t t = a;      \
        a         = b;      \
        b         = t;      \
    }

/// \brief Bresenham's line algorithm from Arduino GFX
/// https://github.com/moononournation/Arduino_GFX/blob/master/src/Arduino_GFX.cpp
/// \param x0 Start X coordinate
/// \param y0 Start Y coordinate
/// \param x1 End X coordinate
/// \param y1 End Y coordinate
/// \param color Line color
/// \details SPI direct write
static void _tft_draw_line_bresenham(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    uint8_t steep = _diff(y1, y0) > _diff(x1, x0);
    if (steep)
    {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }

    if (x0 > x1)
    {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }

    int16_t dx   = x1 - x0;
    int16_t dy   = _diff(y1, y0);
    int16_t err  = dx >> 1;
    int16_t step = (y0 < y1) ? 1 : -1;

    for (; x0 <= x1; x0++)
    {
        if (steep)
        {
            tft_draw_pixel(y0, x0, color);
        }
        else
        {
            tft_draw_pixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0)
        {
            err += dx;
            y0 += step;
        }
    }
}

/// \brief Draw a Rectangle
/// \param x0 Start X coordinate
/// \param y0 Start Y coordinate
/// \param x1 End X coordinate
/// \param y1 End Y coordinate
/// \param color Line color
void tft_draw_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    _tft_draw_fast_h_line(x, y, width, color);
    _tft_draw_fast_h_line(x, y + height - 1, width, color);
    _tft_draw_fast_v_line(x, y, height, color);
    _tft_draw_fast_v_line(x + width - 1, y, height, color);
}

/// \brief Draw Line Function from Arduino GFX
/// https://github.com/moononournation/Arduino_GFX/blob/master/src/Arduino_GFX.cpp
/// \param x0 Start X coordinate
/// \param y0 Start Y coordinate
/// \param x1 End X coordinate
/// \param y1 End Y coordinate
/// \param color Line color
void tft_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    if (x0 == x1)
    {
        if (y0 > y1)
        {
            _swap_int16_t(y0, y1);
        }
        _tft_draw_fast_v_line(x0, y0, y1 - y0 + 1, color);
    }
    else if (y0 == y1)
    {
        if (x0 > x1)
        {
            _swap_int16_t(x0, x1);
        }
        _tft_draw_fast_h_line(x0, y0, x1 - x0 + 1, color);
    }
    else
    {
        _tft_draw_line_bresenham(x0, y0, x1, y1, color);
    }
}
