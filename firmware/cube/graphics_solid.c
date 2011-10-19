/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "graphics.h"


/*
 * Copy vram.color to the LCD bus, and repeat for every pixel.
 */

void vm_solid(void)
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
    __endasm ;

    LCD_WRITE_END();
    lcd_end_frame();
    MODE_RETURN();
}
