/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "graphics.h"

/*
 * BG2: Single 16x16 tiled background, with affine transform.
 *
 * This mode is certainly pushing the limits of the CPU power we have
 * available, and there's no good way to combine it with other modes
 * on the same rendering pass- so this implementation is a bit more
 * monolithic than other modes. No separate scanline renderer, no
 * support for polling for flash updates while we're rendering. More
 * CPU power leftover for BG2. Also this might let us overlay the vars
 * used by BG2 in-between scanlines. We use quite a lot of them.
 *
 * Registers in GFX_BANK:
 *
 *   r0: X accumulator low
 *   r1: X accumulator high
 *   r2: Y accumulator low
 *   r3: Y accumulator high
 *   r4: scratch
 *   r5: Cached pixel address
 *   r6: Cached tile address
 *   r7: Pixel iterator
 *
 * Since this is 8.8 fixed-point, we use r1/r3 as our current pixel
 * location after this accumulator updating is all done with. We
 * calculate a dptr address first, to select a tile from our map:
 *
 *      r1: .xxxxsss
 *      r3: .yyyyttt
 *
 *   r4/r6: yyyyxxxx
 *      
 *     dph: .......y
 *     dpl: yyyxxxx.
 *
 * Then we calculate a low address byte, to select a pixel from the tile:
 *
 * addr/r5: tttsss..
 *
 * If the high bit of r1 or r3 are set, we're past the edge of the map, and
 * we're displaying the border color instead.
 */

// Update the X accumulator, leaving the high byte in 'a'
#define BG2_ACCUM_X()                                          __endasm; \
    __asm mov  a, r0                                           __endasm; \
    __asm add   a, _vm_bg2_xx_1_1                              __endasm; \
    __asm mov   r0, a                                          __endasm; \
    __asm mov   a, r1                                          __endasm; \
    __asm addc  a, (_vm_bg2_xx_1_1 + 1)                        __endasm; \
    __asm mov   r1, a                                          __endasm; \
    __asm

// Update the X accumulator, leaving the high byte in 'a'. Branch if it didn't change.
#define BG2_ACCUM_X_JE(lbl)                                    __endasm; \
    __asm mov  a, r0                                           __endasm; \
    __asm add   a, _vm_bg2_xx_1_1                              __endasm; \
    __asm mov   r0, a                                          __endasm; \
    __asm mov   a, r1                                          __endasm; \
    __asm addc  a, (_vm_bg2_xx_1_1 + 1)                        __endasm; \
    __asm xrl   a, r1                                          __endasm; \
    __asm jz    lbl                                            __endasm; \
    __asm xrl   a, r1                                          __endasm; \
    __asm mov   r1, a                                          __endasm; \
    __asm

// Update the Y accumulator, leaving the high byte in 'a'
#define BG2_ACCUM_Y()                                          __endasm; \
    __asm mov   a, r2                                          __endasm; \
    __asm add   a, _vm_bg2_xy_1_1                              __endasm; \
    __asm mov   r2, a                                          __endasm; \
    __asm mov   a, r3                                          __endasm; \
    __asm addc  a, (_vm_bg2_xy_1_1 + 1)                        __endasm; \
    __asm mov   r3, a                                          __endasm; \
    __asm

// Address a tile (LAT1/LAT2) from 8-bit address in 'a'
#define BG2_ADDR_TILE()                                         __endasm; \
    __asm clr   c                                               __endasm; \
    __asm rlc   a                                               __endasm; \
    __asm mov   dpl, a                                          __endasm; \
    __asm clr   a                                               __endasm; \
    __asm rlc   a                                               __endasm; \
    __asm mov   dph, a                                          __endasm; \
    __asm movx  a, @dptr                                        __endasm; \
    __asm mov   ADDR_PORT, a                                    __endasm; \
    __asm inc   dptr                                            __endasm; \
    __asm mov   CTRL_PORT, #CTRL_FLASH_OUT | CTRL_FLASH_LAT1    __endasm; \
    __asm movx  a, @dptr                                        __endasm; \
    __asm mov   ADDR_PORT, a                                    __endasm; \
    __asm mov   CTRL_PORT, #CTRL_FLASH_OUT | CTRL_FLASH_LAT2    __endasm; \
    __asm

// Address a pixel (r5/ADDR) from the current accumulator state
#define BG2_ADDR_PIXEL()                                        __endasm; \
    __asm mov   a, r3                                           __endasm; \
    __asm swap  a                                               __endasm; \
    __asm rl    a                                               __endasm; \
    __asm anl   a, #0xE0                                        __endasm; \
    __asm mov   r5, a                                           __endasm; \
    __asm mov   a, r1                                           __endasm; \
    __asm rl    a                                               __endasm; \
    __asm rl    a                                               __endasm; \
    __asm anl   a, #0x1c                                        __endasm; \
    __asm orl   a, r5                                           __endasm; \
    __asm mov   r5, a                                           __endasm; \
    __asm mov   ADDR_PORT, a                                    __endasm; \
    __asm

// Address the same pixel we addressed last time
#define BG2_ADDR_RLE_PIXEL()                                    __endasm; \
    __asm mov   ADDR_PORT, r5                                   __endasm; \
    __asm

// Update X tile address, and do X border test
#define BG2_X_UPDATE(lbl)                                       __endasm; \
    __asm rlc   a                                               __endasm; \
    __asm jc    lbl                                             __endasm; \
    __asm swap  a                                               __endasm; \
    __asm anl   a, #0x0F                                        __endasm; \
    __asm mov   r4, a                                           __endasm; \
    __asm

// Update X tile address, without border test
#define BG2_X_UPDATE_NB()                                       __endasm; \
    __asm rlc   a                                               __endasm; \
    __asm swap  a                                               __endasm; \
    __asm anl   a, #0x0F                                        __endasm; \
    __asm mov   r4, a                                           __endasm; \
    __asm

// Update Y tile address, do Y border test, and update tile via cache
#define BG2_Y_UPDATE(borderLbl, cacheHitLbl)                    __endasm; \
    __asm rlc   a                                               __endasm; \
    __asm jc    borderLbl                                       __endasm; \
    __asm anl   a, #0xF0                                        __endasm; \
    __asm orl   a, r4                                           __endasm; \
    __asm xrl   a, r6                                           __endasm; \
    __asm jz    cacheHitLbl                                     __endasm; \
    __asm xrl   a, r6                                           __endasm; \
    __asm mov   r6, a                                           __endasm; \
    __asm BG2_ADDR_TILE()                                       __endasm; \
    __asm cacheHitLbl:                                          __endasm; \
    __asm


void vm_bg2(void)
{
    uint16_t cx;
    uint16_t cy;
    static uint16_t xx;
    static uint16_t xy;
    uint16_t yx;
    uint16_t yy;
    static uint16_t border;

    uint8_t y = vram.num_lines;

    lcd_begin_frame();

    /*
     * Latch all BG2 parameters atomically, at the start of the frame.
     * The 'wiggle' we get when we get a partial matrix update or it's updated
     * partway through a frame is very annoying :)
     */

    cli();
    cx = vram.bg2_affine.cx;
    cy = vram.bg2_affine.cy;
    xx = vram.bg2_affine.xx;
    xy = vram.bg2_affine.xy;
    yx = vram.bg2_affine.yx;
    yy = vram.bg2_affine.yy;
    border = vram.bg2_border;
    sti();
    
    // We pre-increment in the loop below. Compensate by decrementing first.
    cx -= xx;
    cy -= xy;

    /*
     * Initialize tile cache. This stays valid as long as we don't use the
     * flash latches, or r6 in the GFX_BANK. It's unlikely we'll get a hit
     * with this initial value, but there's no way to indicate that the cache
     * is invalid- so it must start out matching our latched address.
     */

    __asm
        mov     a, (GFX_BANK*8 + 6)
        BG2_ADDR_TILE()
    __endasm ;

    /*
     * Loop over scanlines
     */

    do {

        uint16_t __at (GFX_BANK*8 + 0) gfxbank_cx;
        uint16_t __at (GFX_BANK*8 + 2) gfxbank_cy;
        uint8_t __at (GFX_BANK*8 + 7) gfxbank_loops;

        gfxbank_cx = cx;
        gfxbank_cy = cy;
        gfxbank_loops = 128;

        __asm

        mov     psw, #(GFX_BANK << 3)

        ; ---- First pixel (RLE disabled)

        BG2_ACCUM_X()
        BG2_X_UPDATE(12$)
        BG2_ACCUM_Y()
14$:    BG2_Y_UPDATE(13$, 6$)
        BG2_ADDR_PIXEL()
        ASM_ADDR_INC4()
        djnz    r7, 1$
        ljmp    20$

12$:    ljmp    2$
13$:    ljmp    3$

        ; ---- Main pixel loop
1$:
        BG2_ACCUM_X_JE(10$)
        BG2_X_UPDATE(2$)
        BG2_ACCUM_Y()
        BG2_Y_UPDATE(3$, 9$)
        BG2_ADDR_PIXEL()
        ASM_ADDR_INC4()
        djnz    r7, 1$
        sjmp    20$

        ; ---- RLE optimization

10$:
        BG2_ACCUM_Y()
        BG2_ADDR_RLE_PIXEL()
        ASM_ADDR_INC4()
        djnz    r7, 1$
        sjmp    20$

        ; ---- Border pixel rendering

        ; Transition, main -> border

2$:
        BG2_ACCUM_Y()
3$:
        mov     CTRL_PORT, #CTRL_IDLE
        ASM_LCD_WRITE_BEGIN()
        sjmp    8$

        ; Transition, border -> main

4$:
        ASM_LCD_WRITE_END()
        mov     CTRL_PORT, #CTRL_FLASH_OUT

        mov     a, r1
        BG2_X_UPDATE_NB()
        mov     a, r3
        ljmp    14$

        ; Loop 
7$:
        BG2_ACCUM_X()
        BG2_ACCUM_Y()
        orl     a, r1           ; Are we out of the border yet?
        jnb     acc.7, 4$
8$:
        PIXEL_FROM_REGS(_vm_bg2_border_1_1, (_vm_bg2_border_1_1 + 1))
        djnz    r7, 7$
        sjmp    20$

        ; ---- Cleanup

20$:
        ASM_LCD_WRITE_END()
        mov     psw, #0
        __endasm ;

        cx += yx;
        cy += yy;

    } while (--y);    

    CTRL_PORT = CTRL_IDLE;
    lcd_end_frame();
    MODE_RETURN();
}
