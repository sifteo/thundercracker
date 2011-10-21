/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Additional definitions specific to BG1, shared between BG1-derived modes.
 *
 * BG1 has some important differences relative to BG0:
 * 
 *   - It is screen-sized (16x16) rather than 18x18. BG1 is not intended
 *     for 'endless' scrolling, like BG0 is.
 *
 *   - When panned, it does not wrap at the map edge. Pixels from 128
 *     to 255 are transparent in both axes, so the overlay can be smoothly
 *     scrolled into and out of frame.
 *
 *   - Tile addresses in BG1 cannot be computed using a simple multiply,
 *     they must be accumulated by counting '1' bits in the allocation
 *     bitmap.
 *
 * The 8-bit virtual address space is important. Just like BG2, this is a
 * virtual 256x256 space, in which the first quadrant (16x16 tiles) is occupied
 * and all other space is empty. In our case, the empty space corresponds to
 * bits in the BG1 bitmap which are always zero.
 *
 * This means we're shifting 32 bits per line. That would be kind of ridiculous,
 * but we have this handy peripheral to help us out: the Nordic's MDU. This has
 * a 32-bit wide shifter that can shift 2 bits per clock cycle. We keep the current
 * bitmap line in the MDU's 32-bit register.
 */
 
#ifndef _GRAPHICS_BG1_H
#define _GRAPHICS_BG1_H

#include "graphics_bg0.h"

/*
 * Shared state
 */

extern uint8_t x_bg1_offset_x4;         // Panning offset from BG0, multiplied by 4 for jump-table indexing
extern uint8_t x_bg1_shift;             // Amount to shift (ARCON) bitmap words at the start of each line

extern uint8_t y_bg1_addr_l;            // Low part of tile addresses, inc by 32 each line
extern uint8_t y_bg1_bit_index;         // Index into bitmap array, inc by 1 each line
extern uint16_t y_bg1_map;              // Map address for the first tile on this line
extern __bit y_bg1_empty;               // Current line of the bitmap is empty. Set during setup/next.

// Scanline bitmap in MD3..MD0

/*
 * Shared code
 */
 
void vm_bg0_bg1_line(void);
void vm_bg1_setup(void);
void vm_bg1_next(void);

/*
 * Macros
 */

// Load a 16-bit tile address from DPTR without incrementing
#define ADDR_FROM_DPTR(dpl)                                     __endasm; \
    __asm movx  a, @dptr                                        __endasm; \
    __asm mov   ADDR_PORT, a                                    __endasm; \
    __asm inc   dptr                                            __endasm; \
    __asm mov   CTRL_PORT, #CTRL_FLASH_OUT | CTRL_FLASH_LAT1    __endasm; \
    __asm movx  a, @dptr                                        __endasm; \
    __asm mov   ADDR_PORT, a                                    __endasm; \
    __asm dec   dpl                                             __endasm; \
    __asm mov   CTRL_PORT, #CTRL_FLASH_OUT | CTRL_FLASH_LAT2    __endasm; \
    __asm

#endif
