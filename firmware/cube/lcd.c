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


void lcd_init()
{
    // I/O port init
    BUS_DIR = 0xFF;
    ADDR_PORT = 0;
    ADDR_DIR = 0;
    CTRL_PORT = CTRL_IDLE;
    CTRL_DIR = 0x01;

    // Make sure LCD is asleep. This is the default on powerup.
    lcd_sleep();
}

void lcd_sleep()
{
    // Put the LCD into power-saving mode

    LCD_WRITE_BEGIN();
    LCD_CMD_MODE();
    LCD_BYTE(LCD_CMD_SLPIN);
    LCD_BYTE(LCD_CMD_DISPOFF);
    LCD_WRITE_END();
}
    
void lcd_begin_frame()
{
    uint8_t flags = vram.flags;
    uint8_t mode = vram.mode;
    uint8_t first_line = vram.first_line;

    LCD_WRITE_BEGIN();
    LCD_CMD_MODE();

    // Wake up the LCD controller
    LCD_BYTE(LCD_CMD_SLPOUT);

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

    LCD_WRITE_BEGIN();
    LCD_CMD_MODE();
    LCD_BYTE(LCD_CMD_DISPON);
    LCD_WRITE_END();
}
