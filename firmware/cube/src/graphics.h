/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include "lcd.h"
#include "radio.h"

/*
 * Public entry points.
 *
 * Note that to save stack space, we don't actually call graphics_render().
 * We jump there from the main loop, the individual mode function jumps to
 * graphics_ack, then that jumps to graphics_render_ret (back in the main loop)
 */

void graphics_render() __naked;
void graphics_ack() __naked;
void graphics_render_ret() __naked;

#define GRAPHICS_RET()  __asm ljmp _graphics_render_ret __endasm;

/*
 * Video mode entry points
 */

void vm_bg0_rom() __naked;
void vm_solid() __naked;
void vm_fb32() __naked;
void vm_fb64() __naked;
void vm_fb128() __naked;
void vm_bg0() __naked;
void vm_bg0_bg1() __naked;
void vm_bg0_spr_bg1() __naked;
void vm_bg2() __naked;

/*
 * Shared internal definitions
 */

// Pixel plotter exported by FB32
void vm_fb32_pixel() __naked;

// Temporary bank used by some graphics modes
#define GFX_BANK  2

// Output a nonzero number of of pixels, not known at compile-time
#define PIXEL_BURST(_count) {                           \
        register uint8_t _i = (_count);                 \
        do {                                            \
            ADDR_INC4();                                \
        } while (--_i);                                 \
    }

// Output one pixel with static colors from two registers
#define PIXEL_FROM_REGS(l, h)                                   __endasm; \
    __asm mov   BUS_PORT, h                                     __endasm; \
    __asm inc   ADDR_PORT                                       __endasm; \
    __asm inc   ADDR_PORT                                       __endasm; \
    __asm mov   BUS_PORT, l                                     __endasm; \
    __asm inc   ADDR_PORT                                       __endasm; \
    __asm inc   ADDR_PORT                                       __endasm; \
    __asm

// Repeat the same register value for both color bytes
#define PIXEL_FROM_REG(l)                                       __endasm; \
    __asm mov   BUS_PORT, l                                     __endasm; \
    __asm inc   ADDR_PORT                                       __endasm; \
    __asm inc   ADDR_PORT                                       __endasm; \
    __asm inc   ADDR_PORT                                       __endasm; \
    __asm inc   ADDR_PORT                                       __endasm; \
    __asm

#endif
