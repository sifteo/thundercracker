/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Additional definitions specific to BG0, shared between BG0-derived modes.
 */
 
#ifndef _GRAPHICS_BG0_H
#define _GRAPHICS_BG0_H

#include "graphics.h"

/*
 * Shared state
 */

extern uint8_t x_bg0_first_w;           // Width of first displayed background tile, [1, 8]
extern uint8_t x_bg0_last_w;            // Width of first displayed background tile, [0, 7]
extern uint8_t x_bg0_first_addr;        // Low address offset for first displayed tile
extern uint8_t x_bg0_wrap;              // Load value for a dec counter to the next X map wraparound

extern uint8_t y_bg0_addr_l;            // Low part of tile addresses, inc by 32 each line
extern uint16_t y_bg0_map;              // Map address for the first tile on this line

/*
 * Shared code
 */
 
void vm_bg0_line(void);
void vm_bg0_setup(void);
void vm_bg0_next(void);
void vm_bg0_x_wrap_adjust(void) __naked;

/*
 * Macros
 */

// Load a 16-bit tile address from DPTR, and auto-increment
#pragma sdcc_hash +
#define ADDR_FROM_DPTR_INC() {                                  \
    __asm movx  a, @dptr                                        __endasm; \
    __asm mov   ADDR_PORT, a                                    __endasm; \
    __asm inc   dptr                                            __endasm; \
    __asm mov   CTRL_PORT, #CTRL_FLASH_OUT | CTRL_FLASH_LAT1    __endasm; \
    __asm movx  a, @dptr                                        __endasm; \
    __asm mov   ADDR_PORT, a                                    __endasm; \
    __asm inc   dptr                                            __endasm; \
    __asm mov   CTRL_PORT, #CTRL_FLASH_OUT | CTRL_FLASH_LAT2    __endasm; \
    }

// Add 2 to DPTR. (Can do this in 2 clocks with inline assembly)
#define DPTR_INC2() {                                           \
    __asm inc   dptr                                            __endasm; \
    __asm inc   dptr                                            __endasm; \
    }

// Called once per tile, to check for horizontal map wrapping
#define BG0_WRAP_CHECK() {                              \
        if (!--bg0_wrap)                                \
            DPTR -= _SYS_VRAM_BG0_WIDTH *2;             \
    }
        
#define ASM_X_WRAP_CHECK(lbl)                                   __endasm; \
    __asm djnz  r1, lbl                                         __endasm; \
    __asm lcall _vm_bg0_x_wrap_adjust                           __endasm; \
    __asm lbl:                                                  __endasm; \
    __asm

// Assembly macro wrappers
#define ASM_ADDR_INC4()                 __endasm; ADDR_INC4(); __asm
#define ASM_DPTR_INC2()                 __endasm; DPTR_INC2(); __asm
#define ASM_ADDR_FROM_DPTR_INC()        __endasm; ADDR_FROM_DPTR_INC(); __asm

#endif
