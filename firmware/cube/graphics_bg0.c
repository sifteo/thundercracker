/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "graphics_bg0.h"
#include "radio.h"

uint8_t x_bg0_first_w, x_bg0_last_w, x_bg0_first_addr, x_bg0_wrap;
uint8_t y_bg0_addr_l;
uint16_t y_bg0_map;

void vm_bg0_x_wrap_adjust(void) __naked
{
    /*
     * Common routine to adjust DPTR for BG0 horizontal wrap. This doesn't need to be
     * fast, since it happens at most once per scanline.
     */

    __asm
        push    _DPS
        mov     _DPS, #0

        mov     a, dpl
        add     a, #(-_SYS_VRAM_BG0_WIDTH*2)
        mov     dpl, a
        mov     a, dph
        addc    a, #((-_SYS_VRAM_BG0_WIDTH*2) >> 8)
        mov     dph, a

        pop     _DPS
        ret
    __endasm ;
}

void vm_bg0_line(void)
{
    /*
     * Scanline renderer, draws a single tiled background layer.
     */

    uint8_t x = 15;
    uint8_t bg0_wrap = x_bg0_wrap;

    DPTR = y_bg0_map;

    // First partial or full tile
    ADDR_FROM_DPTR_INC();
    BG0_WRAP_CHECK();
    ADDR_PORT = y_bg0_addr_l + x_bg0_first_addr;
    PIXEL_BURST(x_bg0_first_w);

    // There are always 15 full tiles on-screen
    do {
        ADDR_FROM_DPTR_INC();
        BG0_WRAP_CHECK();
        ADDR_PORT = y_bg0_addr_l;
        addr_inc32();
    } while (--x);

    // Might be one more partial tile
    if (x_bg0_last_w) {
        ADDR_FROM_DPTR_INC();
        BG0_WRAP_CHECK();
        ADDR_PORT = y_bg0_addr_l;
        PIXEL_BURST(x_bg0_last_w);
    }
}

void vm_bg0_setup(void)
{
    /*
     * Once-per-frame setup for BG0
     */
     
    uint8_t pan_x, pan_y;
    uint8_t tile_pan_x, tile_pan_y;

    radio_critical_section({
        pan_x = vram.bg0_x;
        pan_y = vram.bg0_y;
    });

    tile_pan_x = pan_x >> 3;
    tile_pan_y = pan_y >> 3;

    y_bg0_addr_l = pan_y << 5;
    y_bg0_map = tile_pan_y << 2;                // Y tile * 2
    y_bg0_map += tile_pan_y << 5;               // Y tile * 16
    y_bg0_map += tile_pan_x << 1;               // X tile * 2;

    x_bg0_last_w = pan_x & 7;
    x_bg0_first_w = 8 - x_bg0_last_w;
    x_bg0_first_addr = (pan_x << 2) & 0x1C;
    x_bg0_wrap = _SYS_VRAM_BG0_WIDTH - tile_pan_x;
}

void vm_bg0_next(void)
{
    /*
     * Advance BG0 state to the next line
     */

    y_bg0_addr_l += 32;
    if (!y_bg0_addr_l) {
        // Next tile, with vertical wrap
        y_bg0_map += _SYS_VRAM_BG0_WIDTH * 2;
        if (y_bg0_map >= _SYS_VRAM_BG0_WIDTH * _SYS_VRAM_BG0_WIDTH * 2)
            y_bg0_map -= _SYS_VRAM_BG0_WIDTH * _SYS_VRAM_BG0_WIDTH * 2;
    }
}

static void vm_bg0(void)
{
    uint8_t y = vram.num_lines;

    lcd_begin_frame();
    vm_bg0_setup();

    do {
        vm_bg0_line();
        vm_bg0_next();
    } while (--y);    

    lcd_end_frame();
}
