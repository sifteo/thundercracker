/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <string.h>
#include "cube_hardware.h"
#include "lcd.h"
#include "flash.h"
#include "time.h"
#include "lcd_model.h"
#include "sensors.h"
#include "radio.h"

__bit lcd_is_awake;


void lcd_reset_delay()
{
    // Arbitrary delay, currently about 50 us.
    uint8_t delay_i = 0;
    do {
    } while (--delay_i);
}


static void lcd_cmd_table(const __code uint8_t *ptr) __naked
{
    /*
     * Table-driven LCD init, so that we don't waste so much ROM
     * space. The table consists of a zero-terminated list of
     * commands:
     *
     *   <total length> <command> <data bytes...>
     *
     * If the length has bit 7 set, it's actually a delay in
     * sensor ticks (6.144 ms) of from 1 to 128 ticks.
     *
     * This is in assembly to save space. (It is not the least
     * bit speed critical, but the compiled code was kind of big)
     */
     
    ptr = ptr;
    __asm
 
        ASM_LCD_WRITE_BEGIN()
     
4$:     clr     a
        movc    a, @a+dptr
        jz      1$              ; Zero, end-of-list
        inc     dptr
        
        jb      acc.7, 2$       ; Delay bit set?
        mov     r0, a           ; No, save as byte count for command
        
        ASM_LCD_CMD_MODE()
3$:     clr     a
        movc    a, @a+dptr
        mov     BUS_PORT, a
        inc     dptr
        lcall   _addr_inc2
        ASM_LCD_DATA_MODE()
        djnz    r0, 3$
        sjmp    4$
        
1$:     ASM_LCD_WRITE_END()
        ret
        
        ; Calculate a target clock tick. We need to subtract 0x80
        ; and add 1 to get the number of ticks, and we need to add
        ; another 1 to reference the _next_ tick instead of the current.

2$:     add     a, #(0x100 - 0x80 + 2)
        add     a, _sensor_tick_counter
        mov     r0, a

5$:     mov     a, _sensor_tick_counter
        xrl     a, r0
        jnz     5$
        sjmp    4$

    __endasm;
}

#ifdef LCD_MODEL_TIANMA_HX8353
void lcd_lut_32()
{
    uint8_t i;
    for (i = 0; i < 31; i++) {
        LCD_BYTE(i << 1);
    }
    LCD_BYTE(63);
}

void lcd_lut_64()
{
    uint8_t i;
    for (i = 0; i < 64; i++) {
        LCD_BYTE(i);
    }
}

void lcd_lut_init()
{
    /*
     * The HX8353 seems to boot up with a totally bogus color LUT.
     * So, we need to set it ourselves.
     */
     
    LCD_WRITE_BEGIN();
    LCD_CMD_MODE();
    LCD_BYTE(LCD_CMD_COLOR_LUT);
    LCD_DATA_MODE();
    
    lcd_lut_32();   // Red
    lcd_lut_64();   // Green
    lcd_lut_32();   // Blue

    // It's okay to return in write mode.
}
#endif

void lcd_sleep()
{
    /*
     * Put the LCD into power-saving mode, if it isn't already.
     */

    if (lcd_is_awake) {
        lcd_is_awake = 0;

        // We have independent control over LCD reset and
        // backlight, via the latches and DCX. First turn off the
        // backlight, then hold the LCD controller in reset mode.
        //
        // Note that, due to this latch arrangement, we can't safely
        // have the LCD off and *not* reset it. By bringing DCX low,
        // we'll cause any other flash operations which may trigger
        // lat1/lat2 to spam BLE and RST with the same value. So, right
        // here, we're free to sequence the turn-off as we see fit, but
        // in the long run BLE and RST need to end up at the same value
        // or we'll see unexpected behaviour when accessing flash memory.

        CTRL_PORT = CTRL_IDLE & ~CTRL_LCD_DCX;
        CTRL_PORT = (CTRL_IDLE & ~CTRL_LCD_DCX) | CTRL_FLASH_LAT1;  // Backlight off
        CTRL_PORT = (CTRL_IDLE & ~CTRL_LCD_DCX) | CTRL_FLASH_LAT2;  // Enter reset
    }   
}
    
void lcd_begin_frame()
{
    uint8_t flags = vram.flags;
    uint8_t mode = vram.mode;

    lcd_window_x = 0;
    lcd_window_y = vram.first_line;

    /*
     * Wake up the LCD controller, if necessary.
     */
    if (!lcd_is_awake) {
        lcd_is_awake = 1;
        
        // First, we take the LCD controller out of reset.
        // Then we fully initialize it. We turn on the backlight
        // after both initialization is complete and the first
        // frame has finished rendering.

        CTRL_PORT = CTRL_IDLE & ~CTRL_LCD_DCX;
        CTRL_PORT = (CTRL_IDLE & ~CTRL_LCD_DCX) | CTRL_FLASH_LAT2;  // Enter reset

        lcd_reset_delay();

        CTRL_PORT = CTRL_IDLE;
        CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;  // Exit reset
        
        // Controller initialization
        lcd_cmd_table(lcd_setup_table);
        #ifdef LCD_MODEL_TIANMA_HX8353
        lcd_lut_init();
        #endif
    }

    LCD_WRITE_BEGIN();

    // Set addressing mode
    LCD_CMD_MODE();
    LCD_BYTE(LCD_CMD_MADCTR);
    LCD_DATA_MODE();
    LCD_BYTE(LCD_MADCTR_NORMAL ^ (flags & LCD_MADCTR_VRAM));

    // Set row/column addressing, and start RAMWR.
    lcd_address_and_write();

    LCD_WRITE_END();

    // Vertical sync
    #ifdef HAVE_LCD_TE
    if (flags & _SYS_VF_SYNC)
        while (!CTRL_LCD_TE);
    #endif
}    


void lcd_end_frame()
{
    /*
     * After a completed frame, we can turn on the LCD to show that
     * frame. This has no effect if we aren't just now coming back
     * from display sleep, but if we are, this prevents showing one
     * garbage frame prior to showing the intended frame.
     *
     * It's important that we issue *some* kind of command here, to
     * take us out of RAMWR mode. This way, any spurious Write strobes
     * (e.g. from battery voltage sensing) won't generate pixels.
     *
     * Implies LCD_WRITE_END(), since this commonly occurs right before
     * lcd_end_frame on framebuffer modes, and is harmless on other modes.
     */

    static const __code uint8_t table[] = {
        2, LCD_CMD_DISPON, 0x00,
        0,
    };

    // Release the bus
    LCD_WRITE_END();
    CTRL_PORT = CTRL_IDLE;
    
    lcd_cmd_table(table);

    /*
     * Now that the LCD is fully ready, turn on the backlight if it isn't
     * already on. Note that rendering from flash can cause this to happen
     * earlier, but in BG0_ROM mode we can successfully delay backlight enable
     * until after the first frame has fully rendered.
     */
    CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT1;
}

void lcd_address_and_write(void)
{
    /*
     * Change the row/column address, and start a RAMWR command.
     *
     * Assumes we're already set up for writing to the LCD.
     * Leaves with the bus in data mode.
     */

    LCD_CMD_MODE();
    LCD_BYTE(LCD_CMD_CASET);
    LCD_DATA_MODE();
    LCD_BYTE(0);
    LCD_BYTE(LCD_COL_ADDR(lcd_window_x));
    LCD_BYTE(0);
    LCD_BYTE(LCD_COL_ADDR(LCD_WIDTH - 1));

    LCD_CMD_MODE();
    LCD_BYTE(LCD_CMD_RASET);
    LCD_DATA_MODE();
    LCD_BYTE(0);
    LCD_BYTE(LCD_ROW_ADDR(lcd_window_y));
    LCD_BYTE(0);
    LCD_BYTE(LCD_ROW_ADDR(LCD_HEIGHT - 1));

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

    LCD_CMD_MODE();
    LCD_BYTE(LCD_CMD_NOP);
    ADDR_PORT = 1;
    LCD_BYTE(LCD_CMD_RAMWR);
    LCD_DATA_MODE();
}

void vram_atomic_copy() __naked
{
    // dptr=src, r0=dest, r1=count.
    // Only trashes a, r0, r1, and dptr.

    radio_irq_disable();
    __asm
1$:
        movx    a, @dptr
        mov     @r0, a
        inc     dptr
        inc     r0
        djnz    r1, 1$
    __endasm;
    radio_irq_enable();

    __asm    
        ret
    __endasm;
}
