/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "graphics.h"

/*
 * 16-color 32x32 framebuffer mode.
 *
 * 16-entry colormap, 16 bytes per line.
 *
 * To support fast color lookups without copying the whole LUT into
 * internal memory, we use both DPTRs here.
 */

void vm_fb32_pixel() __naked
{
    __asm
        mov     _DPS, #1

        anl     a, #0xF
        rl      a
        mov     _DPL1, a
        movx    a, @dptr
        mov     r0, a
        inc     dptr
        movx    a, @dptr

        PIXEL_FROM_REGS(r0, a)

        mov     _DPS, #0
        ret
    __endasm;
}

static void vm_fb32_pixel3() __naked
{
    __asm
        PIXEL_FROM_REGS(r0, a)
        PIXEL_FROM_REGS(r0, a)
        PIXEL_FROM_REGS(r0, a)
        ret
    __endasm;
}

static void vm_fb32_line(uint16_t src)
{
    src = src;
    __asm
        mov     r4, #16         ; Loop over 16 horizontal bytes per line
4$:

        movx    a, @dptr
        inc     dptr
        mov     r5, a

        ; Low nybble

        lcall   _vm_fb32_pixel
        lcall   _vm_fb32_pixel3

        ; High nybble

        mov     a, r5
        swap    a
        lcall   _vm_fb32_pixel
        lcall   _vm_fb32_pixel3

        djnz    r4, 4$          ; Next byte

    __endasm ;
}

void vm_fb32(void)
{
    lcd_begin_frame();
    LCD_WRITE_BEGIN();

    {
        uint8_t y = vram.num_lines;
        uint16_t src = 0;

        do {
            vm_fb32_line(src);
            if (!--y) break;
            vm_fb32_line(src);
            if (!--y) break;
            vm_fb32_line(src);
            if (!--y) break;
            vm_fb32_line(src);
            src += 16;
            src &= 0x1F0;
        } while (--y);    
    }

    lcd_end_frame();
}
