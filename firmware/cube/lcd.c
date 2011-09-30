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
#include "lcd_model.h"

static bit lcd_is_awake;


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

    // Set addressing mode
    LCD_BYTE(LCD_CMD_MADCTR);
    LCD_DATA_MODE();
    LCD_BYTE(LCD_MADCTR_NORMAL | (flags & LCD_MADCTR_VRAM));
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
        0,
    };

    lcd_cmd_table(table);
}
