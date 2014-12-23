/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * Micah Elizabeth Scott <micah@misc.name>
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
        mov     r2, #16         ; Loop over 16 horizontal bytes per line
4$:

        movx    a, @dptr
        inc     dptr
        mov     r3, a

        ; Low nybble

        acall   _vm_fb32_pixel
        acall   _vm_fb32_pixel3

        ; High nybble

        mov     a, r3
        swap    a
        acall   _vm_fb32_pixel
        acall   _vm_fb32_pixel3

        djnz    r2, 4$          ; Next byte

    __endasm ;
}

void vm_fb32(void) __naked
{
    lcd_begin_frame();
    LCD_WRITE_BEGIN();

    __asm

        ; Keep line count in r7

        mov     dptr, #_SYS_VA_NUM_LINES
        movx    a, @dptr
        mov     r7, a

        clr     a
        mov     r5, a               ; Line pointer in r6:r5
        mov     r6, a
        mov     r4, a               ; Vertical scaling counter in r4

1$:
        mov     dpl, r5             ; Draw line from r6:r5
        mov     dph, r6
        acall   _vm_fb32_line

        mov     a, r4               ; Next source line? (Group of 4 dest lines)
        inc     a
        mov     r4, a
        anl     a, #3
        jnz     2$

        mov     a, r5               ; Add 16 bytes
        add     a, #16
        mov     r5, a
        mov     a, r6
        addc    a, #0
        anl     a, #1               ; Mask to 0x1FF
        mov     r6, a

2$:
        djnz    r7, 1$              ; Next line. Done yet?

    __endasm;

    lcd_end_frame();
    GRAPHICS_RET();
}
