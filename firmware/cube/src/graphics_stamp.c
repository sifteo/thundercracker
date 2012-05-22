/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
        ; GFX_BANK, and it does not dirty any registers other than a, r0, r1.
        mov     _lcd_window_x, r5
        lcall   _lcd_address_and_write
        inc     r5

        mov     _DPS, #1                ; Load colormap index, via DPTR1
        rl      a
        mov     _DPL1, a
        movx    a, @dptr
        mov     r0, a
        inc     dptr
        movx    a, @dptr
        mov     _DPS, #0

        PIXEL_FROM_REGS(r0, a)          ; Draw a single pixel

        ret
    __endasm ;
}

static void vm_stamp_line(uint16_t src) __naked __using(GFX_BANK)
{
    src = src;
    __asm
        mov     _DPH1, #(_SYS_VA_COLORMAP >> 8)
        mov     psw, #(GFX_BANK << 3)

        ; Reuse height reg as pitch backup
        mov     r4, _gfxbank_pitch

        ; Bottom half
1$:
        anl     dph, #3         ; Mask to VRAM size
        movx    a, @dptr
        inc     dptr
        mov     r2, a

        lcall   _vm_stamp_pixel

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

        lcall   _vm_stamp_pixel

        djnz    r6, 1$          ; Next pixel

        ; Clean up
3$:
        mov     psw, #0
        ret

    __endasm ;
}

// Latch parameters, copying them from VRAM to GFX_BANK.
// Trashes dptr, r0, r1, and a.
static void vm_stamp_latch_parameters() __naked
{
    __asm
        mov     dptr, #_SYS_VA_STAMP_PITCH
        mov     r0, #_gfxbank_pitch
        mov     r1, #5
        ljmp    _vram_atomic_copy
    __endasm ;
}

void vm_stamp(void)
{
    uint8_t y = vram.num_lines;
    uint8_t heightCounter;
    uint16_t src = 0;

    vm_stamp_latch_parameters();
    heightCounter = gfxbank_height;

    lcd_begin_frame();
    LCD_WRITE_BEGIN();

    do {
        vm_stamp_line(src);
        vm_stamp_latch_parameters();

        if (--heightCounter) {
            src += gfxbank_pitch;
        } else {
            heightCounter = gfxbank_height;
            src = 0;
        }

        lcd_window_y++;

    } while (--y);

    LCD_WRITE_END();
    lcd_end_frame();
}
