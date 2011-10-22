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

static void vm_fb32_line(uint16_t src)
{
    src = src;
    __asm
        mov     _DPH1, #(_SYS_VA_COLORMAP >> 8)
        
        mov     r4, #16         ; Loop over 16 horizontal bytes per line
4$:

        movx    a, @dptr
        inc     dptr
        mov     r5, a
        mov     _DPS, #1

        ; Low nybble

        anl     a, #0xF
        rl      a
        mov     _DPL1, a
        movx    a, @dptr
        mov     r0, a
        inc     dptr
        movx    a, @dptr

        PIXEL_FROM_REGS(r0, a)
        PIXEL_FROM_REGS(r0, a)
        PIXEL_FROM_REGS(r0, a)
        PIXEL_FROM_REGS(r0, a)

        ; High nybble

        mov     a, r5
        anl     a, #0xF0
        swap    a
        rl      a
        mov     _DPL1, a
        movx    a, @dptr
        mov     r0, a
        inc     dptr
        movx    a, @dptr

        PIXEL_FROM_REGS(r0, a)
        PIXEL_FROM_REGS(r0, a)
        PIXEL_FROM_REGS(r0, a)
        PIXEL_FROM_REGS(r0, a)

        mov     _DPS, #0
        djnz    r4, 4$          ; Next byte

    __endasm ;
}

void vm_fb32(void)
{
    uint8_t y = vram.num_lines;
    uint16_t src = 0;

    lcd_begin_frame();
    LCD_WRITE_BEGIN();

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

    LCD_WRITE_END();
    lcd_end_frame();
}
