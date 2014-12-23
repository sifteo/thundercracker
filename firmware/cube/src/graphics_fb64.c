/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "graphics.h"

extern uint8_t fb64_y;
extern uint8_t fb64_scale;

/*
 * 2-color 64x64 framebuffer mode.
 *
 * Two-entry colormap, 8 bytes per line.
 * Clobbers r2-r7.
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

void vm_fb64(void) __naked
{
    lcd_begin_frame();
    LCD_WRITE_BEGIN();

    __asm

        ; Keep line count in fb64_y

        mov     dptr, #_SYS_VA_NUM_LINES
        movx    a, @dptr
        mov     _fb64_y, a

        clr     a
        mov     r1, a               ; Line pointer in r1:r0
        mov     r0, a
        mov     _fb64_scale, a      ; Vertical scaling counter in r4

1$:
        mov     dpl, r0             ; Draw line from r1:r0
        mov     dph, r1
        acall   _vm_fb64_line

        mov     a, _fb64_scale      ; Next source line? (Group of 2 dest lines)
        inc     a
        mov     _fb64_scale, a
        anl     a, #1
        jnz     2$

        mov     a, r0               ; Add 8 bytes
        add     a, #8
        mov     r0, a
        mov     a, r1
        addc    a, #0
        anl     a, #1               ; Mask to 0x1FF
        mov     r1, a

2$:
        djnz    _fb64_y, 1$         ; Next line. Done yet?

    __endasm;

    lcd_end_frame();
    GRAPHICS_RET();
}
