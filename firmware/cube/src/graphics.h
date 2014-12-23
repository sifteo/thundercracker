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

#define GRAPHICS_RET()      __asm ljmp _graphics_ack __endasm;
#define GRAPHICS_ARET()     __asm ajmp _graphics_ack __endasm;

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
