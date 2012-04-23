/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "graphics.h"

/*
 * 2-color 64x64 framebuffer mode.
 *
 * Two-entry colormap, 8 bytes per line.
 */

void vm_fb64_line(uint16_t ptr)
{
    ptr = ptr;
    __asm

        ; Copy colormap[0] and colormap[1] to r4-7

        mov     _DPS, #1
        mov     dptr, #_SYS_VA_COLORMAP
        movx    a, @dptr
        mov     r4, a
        inc     dptr
        movx    a, @dptr
        mov     r5, a
        inc     dptr
        movx    a, @dptr
        mov     r6, a
        inc     dptr
        movx    a, @dptr
        mov     r7, a
        mov     _DPS, #0

        mov     r2, #8          ; Loop over 8 horizontal bytes per line
4$:     movx    a, @dptr
        inc     dptr
        mov     r3, #8          ; Loop over 8 pixels per byte
5$:     rrc     a               ; Shift out one pixel
        jc      6$

        PIXEL_FROM_REGS(r4, r5) ; colormap[0]
        PIXEL_FROM_REGS(r4, r5)
        djnz    r3, 5$          ; Next pixel
        djnz    r2, 4$          ; Next byte
        sjmp    7$

6$:     PIXEL_FROM_REGS(r6, r7) ; colormap[1]
        PIXEL_FROM_REGS(r6, r7)
        djnz    r3, 5$          ; Next pixel
        djnz    r2, 4$          ; Next byte

7$:
 
    __endasm ;
}

void vm_fb64(void)
{
    uint8_t y = vram.num_lines;
    uint16_t src = 0;

    lcd_begin_frame();
    LCD_WRITE_BEGIN();

    do {
        vm_fb64_line(src);
        if (!--y) break;
        vm_fb64_line(src);
        src += 8;
        src &= 0x1F8;
    } while (--y);    

    LCD_WRITE_END();
    lcd_end_frame();
}
