/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <string.h>
#include "hardware.h"
#include "lcd.h"
#include "flash.h"
#include "time.h"

static bit lcd_is_awake;

/*
 * Initial poweron or wake-time setup for the LCD controller.
 *
 * XXX: This is mostly copied from the gen1 cube firmware
 *      ILI9163C_Initialize. Some of it may not be
 *      applicable/correct, or may be wasteful.
 */
static const __code uint8_t lcd_setup_table[] =
{
    // Reset
    1, 0x01, 0x00,

    // Short delay
    0x80,

    1, 0xec, 0x1b,

    // sleep out
    1, 0x11, 0x00,

    // Short delay
    0x80,

    1, 0x26, 0x01,

    4, 0x2a, 0x00, 0x00, 0x00, 0x7f,
    4, 0x2b, 0x00, 0x00, 0x00, 0x7f,

    // TE On
    1, 0x35, 0x00,

    1, 0x36, 0x08,

    // Frame rate control
    2, 0xb1, 0x0c, 0x03,

    // Display Function set,
    // NO[5:4], SDT[3:2], EQ[1:0]
    // PTG[3:2], PT[1:0]
    2, 0xb6, 0x07, 0x02,

    // GVDD
    2, 0xc0, 0x08, 0x00,

    // AVDD VGL VGL VCL
    1, 0xc1, 0x03,

    2, 0xc2, 0x03, 0x03,

    // SET VCOMH & VCOML
    2, 0xc5, 0x4a, 0x3e,

    // VCOM Offset control
    1, 0xc7, 0x40,

    // Gamma set E0.E1 enable control
    1, 0xf2, 0x01,

    // Gamma
    15, 0xe0, 0x3f, 0x2a, 0x27, 0x2d, 0x26, 0x0c, 0x53, 0xf4, 0x3f, 0x16, 0x1d, 0x15, 0x0f, 0x06, 0x00,
    15, 0xe1, 0x00, 0x15, 0x18, 0x11, 0x19, 0x13, 0x2b, 0x63, 0x40, 0x09, 0x22, 0x2a, 0x30, 0x39, 0x3f,

    1, 0x29, 0x00,

    // delay 120msec
    0xFF,

    0,
};


void lcd_init()
{
    /*
     * I/O port init
     */

    BUS_DIR = 0xFF;
    ADDR_PORT = 0;
    ADDR_DIR = 0;
    CTRL_PORT = CTRL_IDLE;
    CTRL_DIR = CTRL_DIR_VALUE;
}

static void lcd_cmd_table(const __code uint8_t *ptr)
{
    /*
     * Table-driven LCD init, so that we don't waste so much ROM
     * space. The table consists of a zero-terminated list of
     * commands:
     *
     *   <data length> <command> <data bytes...>
     *
     * If the length has bit 7 set, it's actually a delay in
     * milliseconds, from 1 to 128 ms.
     */
    
    uint8_t len;

    LCD_WRITE_BEGIN();

    while ((len = *ptr)) {
	ptr++;

	if (len & 0x80) {
	    msleep(len - 0x7F);

	} else {
	    LCD_CMD_MODE();
	    LCD_BYTE(*ptr);
	    LCD_DATA_MODE();
	    ptr++;

	    do {
		LCD_BYTE(*ptr);
		ptr++;
	    } while (--len);
	}
    }

    LCD_WRITE_END();
}

void lcd_sleep()
{
    /*
     * Put the LCD into power-saving mode, if it isn't already.
     */

    if (lcd_is_awake) {

	static const __code uint8_t table[] = {
	    1, LCD_CMD_SLPIN, 0x00,
	    1, LCD_CMD_DISPOFF, 0x00,
	    0,
	};

	lcd_is_awake = 0;
	lcd_cmd_table(table);
    }	
}
    
void lcd_begin_frame()
{
    uint8_t flags = vram.flags;
    uint8_t mode = vram.mode;
    uint8_t first_line = vram.first_line;

    /*
     * Wake up the LCD controller, if necessary.
     */
    if (!lcd_is_awake) {
	lcd_is_awake = 1;
	lcd_cmd_table(lcd_setup_table);
    }

    LCD_WRITE_BEGIN();
    LCD_CMD_MODE();

    // Use 16-bit color mode
    LCD_BYTE(LCD_CMD_COLMOD);
    LCD_DATA_MODE();
    LCD_BYTE(LCD_COLMOD_16);
    LCD_CMD_MODE();

    // Set addressing mode
    LCD_BYTE(LCD_CMD_MADCTR);
    LCD_DATA_MODE();
    LCD_BYTE(flags & (LCD_MADCTR_MY | LCD_MADCTR_MX | LCD_MADCTR_MV));
    LCD_CMD_MODE();

    // Set the row address to first_line
    LCD_BYTE(LCD_CMD_RASET);
    LCD_DATA_MODE();
    LCD_BYTE(0);
    LCD_BYTE(first_line);
    LCD_BYTE(0);
    LCD_BYTE(LCD_HEIGHT - 1);
    LCD_CMD_MODE();

    // Start writing
    LCD_BYTE(LCD_CMD_RAMWR);

    LCD_DATA_MODE();
    LCD_WRITE_END();

    // Vertical sync
    if (flags & _SYS_VF_SYNC)
	while (!CTRL_LCD_TE);
}    


void lcd_end_frame()
{
    /*
     * After a completed frame, we can turn on the LCD to show that
     * frame. This has no effect if we aren't just now coming back
     * from display sleep, but if we are, this prevents showing one
     * garbage frame prior to showing the intended frame.
     */

    static const __code uint8_t table[] = {
	1, LCD_CMD_DISPON, 0x00,
    };

    lcd_cmd_table(table);
}
