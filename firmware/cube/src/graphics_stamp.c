/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc.
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
 * Reconfigurable 16-color framebuffer mode, with transparency.
 *
 * We use the 16-entry colormap, just like FB32. But, unlike FB32:
 *
 *   - We don't do any scaling
 *   - Pixels matching a "key" index are skipped entirely
 *   - The framebuffer geometry is configurable
 *   - Performance is much less of a priority
 *
 * The framebuffer is configured with five parameters, in VRAM, in addition
 * to the mode-independent windowing settings:
 *
 *   - pitch:  The width of each framebuffer line, in bytes.
 *             If width > 2*pitch, the image repeats horizontally.
 *   - height: Number of image rows to draw before repeating
 *   - x:      First column of the LCD to draw to
 *   - width:  Number of LCD columns to (potentially) draw to (> 0)
 *   - key:    Transparent color index, if in range [0, 15]. Otherwise,
 *             this has the effect of disabling transparency.
 */

// Copy of VRAM parameters, in GFX_BANK
static uint8_t __at (GFX_BANK*8 + 3) gfxbank_pitch;
static uint8_t __at (GFX_BANK*8 + 4) gfxbank_height;
static uint8_t __at (GFX_BANK*8 + 5) gfxbank_x;
static uint8_t __at (GFX_BANK*8 + 6) gfxbank_width;
static uint8_t __at (GFX_BANK*8 + 7) gfxbank_key;

static void vm_stamp_pixel() __naked __using(GFX_BANK)
{
    __asm
        anl     a, #0xF                 ; Mask off the nybble we want
        cjne    a, _gfxbank_key, 1$     ; Colorkey test
        inc     r5
        ret
1$:

        ; Fragile assumption... lcd_address_and_write is safe to run in
        ; GFX_BANK, and it does not dirty any registers other than r0 and r1.
        ; Notably, the usage of vm_fb32_pixel() below assumes that a will
        ; be preserved.
        mov     _lcd_window_x, r5
        lcall   _lcd_address_and_write
        inc     r5

        ajmp    _vm_fb32_pixel

    __endasm ;
}

extern void vm_stamp_latch_parameters() __naked;
static void vm_stamp_line(uint16_t src) __naked __using(GFX_BANK)
{
    src = src;
    __asm
        mov     psw, #(GFX_BANK << 3)

        ; Reuse height reg as pitch backup
        mov     r4, _gfxbank_pitch

        ; Bottom half
1$:
        anl     dph, #3         ; Mask to VRAM size
        movx    a, @dptr
        inc     dptr
        mov     r2, a

        acall   _vm_stamp_pixel

        djnz    r3, 4$          ; Pitch wrap?
        mov     r3, _gfxbank_height
        mov     a, dpl          ; Step backwards
        clr     c
        subb    a, r4
        mov     dpl, a
        mov     a, dph
        subb    a, #0
        mov     dph, a
4$:

        djnz    r6, 2$          ; Next pixel
        sjmp    3$

        ; Top half
2$:
        mov     a, r2
        swap    a

        acall   _vm_stamp_pixel

        djnz    r6, 1$          ; Next pixel

        ; Clean up, fall through to re-latch parameters
3$:
        mov     psw, #0
 
        ; Latch parameters, copying them from VRAM to GFX_BANK.
        ; Trashes dptr, r0, r1, and a.

_vm_stamp_latch_parameters:
        mov     dptr, #_SYS_VA_STAMP_PITCH
        mov     r0, #_gfxbank_pitch
        mov     r1, #5
        ljmp    _vram_atomic_copy

    __endasm ;
}

void vm_stamp(void) __naked
{
    lcd_begin_frame();
    LCD_WRITE_BEGIN();
    vm_stamp_latch_parameters();

    {
        uint8_t y = vram.num_lines;
        uint8_t heightCounter;
        uint16_t src = 0;

        heightCounter = gfxbank_height;

        do {
            vm_stamp_line(src);

            if (!--heightCounter) {
                heightCounter = gfxbank_height;
                src = 0;
            } else {
                src += gfxbank_pitch;
            }

            lcd_window_y++;

        } while (--y);
    }

    lcd_end_frame();
    GRAPHICS_RET();
}
