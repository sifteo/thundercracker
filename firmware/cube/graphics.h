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
 * Public entry point
 */

void graphics_render() __naked;

/*
 * Video mode entry points
 */
 
void vm_powerdown(void);
void vm_bg0_rom();
void vm_solid();
void vm_fb32();
void vm_fb64();
void vm_fb128();
void vm_bg0();
void vm_bg0_bg1();
void vm_bg0_spr_bg1();
void vm_bg2();

/*
 * Shared internal definitions
 */

// Temporary bank used by some graphics modes
#define GFX_BANK  2

// Code to run just prior to returning from a video mode
#pragma sdcc_hash +
#define MODE_RETURN() {                                         \
                __asm   inc     (_ack_data + RF_ACK_FRAME)      __endasm ; \
                __asm   orl     _ack_len, #RF_ACK_LEN_FRAME     __endasm ; \
                return;                                         \
        }

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
