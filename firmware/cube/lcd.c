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

static __bit lcd_is_awake;


static void lcd_cmd_table(const __code uint8_t *ptr)
{
    /*
     * Table-driven LCD init, so that we don't waste so much ROM
     * space. The table consists of a zero-terminated list of
     * commands:
     *
     *   <total length> <command> <data bytes...>
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
            do {
                LCD_BYTE(*ptr);
                LCD_DATA_MODE();
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
     *
     * We also must turn on the backlight, and we must do this
     * before running the initialization sequence. See the
     * comments on the ST7735 controller, in lcd_model.h.
     */
    if (!lcd_is_awake) {
        CTRL_PORT = CTRL_IDLE;  // Backlight on
        lcd_is_awake = 1;
        lcd_cmd_table(lcd_setup_table);
    }

    LCD_WRITE_BEGIN();
    LCD_CMD_MODE();

    // Set addressing mode
    LCD_BYTE(LCD_CMD_MADCTR);
    LCD_DATA_MODE();
    LCD_BYTE(LCD_MADCTR_NORMAL ^ (flags & LCD_MADCTR_VRAM));
    LCD_CMD_MODE();

    // Set the row address to first_line
    LCD_BYTE(LCD_CMD_RASET);
    LCD_DATA_MODE();
    LCD_BYTE(0);
    LCD_BYTE(LCD_ROW_ADDR(first_line));
    LCD_BYTE(0);
    LCD_BYTE(LCD_ROW_ADDR(LCD_HEIGHT - 1));
    LCD_CMD_MODE();

    /*
     * Start writing (RAMWR command).
     *
     * We have to do this particular command -> data transition
     * somewhat gingerly, since the Truly LCD is really picky. If we
     * transition from command to data while the write strobe is low,
     * it will falsely detect that as an additional byte-write that we
     * didn't intend.
     *
     * This paranoia is unnecessary but harmless on the Giantplus LCD.
     */

    LCD_BYTE(LCD_CMD_NOP);
    ADDR_PORT = 1;
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

    // Release the bus
    CTRL_PORT = CTRL_IDLE;
    
    lcd_cmd_table(table);
    
    // Acknowledge this frame
    __asm
        inc     (_ack_data + RF_ACK_FRAME)
        orl     _ack_len, #RF_ACK_LEN_FRAME
    __endasm ;
}


void addr_inc32() __naked {
    /*
     * Common jump table for address increments.
     * (Yes, this is a C function with entry points in the middle...)
     */

    __asm
            .globl _addr_inc28
            .globl _addr_inc24
            .globl _addr_inc20
            .globl _addr_inc16
            .globl _addr_inc12
            .globl _addr_inc8
            .globl _addr_inc4
    
             ASM_ADDR_INC4()
_addr_inc28: ASM_ADDR_INC4()
_addr_inc24: ASM_ADDR_INC4()
_addr_inc20: ASM_ADDR_INC4()
_addr_inc16: ASM_ADDR_INC4()
_addr_inc12: ASM_ADDR_INC4()
_addr_inc8:  ASM_ADDR_INC4()
_addr_inc4:  ASM_ADDR_INC4()
             ret

    __endasm ;
}
