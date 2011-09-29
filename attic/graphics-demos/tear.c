/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Try to provoke LCD tearing
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "hardware.h"

static void lcd_addr_burst(uint8_t pixels)
{
    uint8_t hi = pixels >> 3;
    uint8_t low = pixels & 0x7;

    if (hi)
        // Fast DJNZ loop, 8-pixel bursts
        do {
            ADDR_INC32();
        } while (--hi);

    if (low)
        // Fast DJNZ loop, single pixels
        do {
            ADDR_INC4();
        } while (--low);
}

void lcd_cmd_byte(uint8_t b)
{
    CTRL_PORT = CTRL_LCD_CMD;
    BUS_DIR = 0;
    BUS_PORT = b;
    ADDR_INC2();
    BUS_DIR = 0xFF;
    CTRL_PORT = CTRL_IDLE;
}

void lcd_data_byte(uint8_t b)
{
    BUS_DIR = 0;
    BUS_PORT = b;
    ADDR_INC2();
    BUS_DIR = 0xFF;
}

void main(void)
{
    uint8_t y;
    uint8_t color = 0;

    // I/O port init
    BUS_DIR = 0xFF;
    ADDR_PORT = 0;
    ADDR_DIR = 0;
    CTRL_PORT = CTRL_IDLE;
    CTRL_DIR = 0x01;

    lcd_cmd_byte(LCD_CMD_SLPOUT);
    lcd_cmd_byte(LCD_CMD_DISPON);
    lcd_cmd_byte(LCD_CMD_TEON);

    lcd_cmd_byte(LCD_CMD_COLMOD);
    lcd_data_byte(LCD_COLMOD_16);

    while (1) {
        color ^= 0xFF;

        // Sync with LCD
        while (!CTRL_LCD_TE);

        // Fill screen

        lcd_cmd_byte(LCD_CMD_RAMWR);
        BUS_DIR = 0;
        BUS_PORT = color;
        y = LCD_HEIGHT;
        do {
            lcd_addr_burst(LCD_WIDTH);
        } while (--y);
        BUS_DIR = 0xFF;
    }
}
