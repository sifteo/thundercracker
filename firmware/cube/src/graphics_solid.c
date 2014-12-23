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
 * Copy vram.color to the LCD bus, and repeat for every pixel.
 */

void vm_solid(void) __naked
{
    lcd_begin_frame();
    LCD_WRITE_BEGIN();

    __asm
        mov     dptr, #_SYS_VA_NUM_LINES
        movx    a, @dptr
        mov     r1, a

        mov     dptr, #_SYS_VA_COLORMAP
        movx    a, @dptr
        mov     r0, a
        inc     dptr
        movx    a, @dptr

1$:     mov     r2, #LCD_WIDTH
2$:

        PIXEL_FROM_REGS(r0, a)

        djnz    r2, 2$
        djnz    r1, 1$

        ljmp    lcd_end_frame_and_graphics_ret

    __endasm ;
}
