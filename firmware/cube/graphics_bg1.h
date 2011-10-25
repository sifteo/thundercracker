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

extern __bit x_bg1_offset_bit0;         // Panning offset from BG0, as three separate bits.
extern __bit x_bg1_offset_bit1;
extern __bit x_bg1_offset_bit2;

extern uint8_t x_bg1_first_addr;        // X portion of address for first pixel
extern uint8_t x_bg1_last_addr;         // X portion of address for last tile

extern uint8_t y_bg1_addr_l;            // Low part of tile addresses, inc by 32 each line
extern uint8_t y_bg1_bit_index;         // Index into bitmap array, inc by 1 each line
extern uint16_t y_bg1_map;              // Map address for the first tile on this line
extern __bit y_bg1_empty;               // Current line of the bitmap is empty. Set during setup/next.

// Scanline bitmap in MD3..MD0

/*
 * Scanline renderer registers
 */

#define R_X_WRAP        r1
#define R_BG0_ADDR      r3
#define R_BG1_ADDR      r4
#define R_LOOP_COUNT    r5

/*
 * Shared code
 */
 
void vm_bg0_bg1_line(void);
void vm_bg1_setup(void);
void vm_bg1_next(void);

// Low-level renderers
void vm_bg0_bg1_pixels(void) __naked;
void vm_bg0_bg1_tiles_fast(void);

void state_bg0_0(void) __naked;
void state_bg1_0(void) __naked;

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
  
// Next BG1 tile (while using DPTR1)
#define ASM_BG1_NEXT()                                          __endasm; \
    __asm inc   dptr                                            __endasm; \
    __asm inc   dptr                                            __endasm; \
    __asm anl   _DPH1, #3                                       __endasm; \
    __asm    

#define BG1_NEXT_BIT()                                          __endasm; \
    __asm mov   _MD0, b                                         __endasm; \
    __asm mov   _ARCON, #0x21                                   __endasm; \
    __asm

#define BG1_UPDATE_BIT()                                        __endasm; \
    __asm mov   b, _MD0                                         __endasm; \
    __asm
    
#define BG1_JB(lbl)                                             __endasm; \
    __asm jb    b.0, lbl                                        __endasm; \
    __asm

#define BG1_JNB(lbl)                                            __endasm; \
    __asm jnb    b.0, lbl                                       __endasm; \
    __asm

#define BG1_JB_L(lbl)                                           __endasm; \
    __asm jnb    b.0, (.+6)                                     __endasm; \
    __asm ljmp   lbl                                            __endasm; \
    __asm

#define BG1_JNB_L(lbl)                                          __endasm; \
    __asm jb     b.0, (.+6)                                     __endasm; \
    __asm ljmp   lbl                                            __endasm; \
    __asm
    
#define CHROMA_PREP()                                           __endasm; \
    __asm mov   a, #_SYS_CHROMA_KEY                             __endasm; \
    __asm

#define CHROMA_J_OPAQUE(lbl)                                    __endasm; \
    __asm cjne  a, BUS_PORT, lbl                                __endasm; \
    __asm

#define CHROMA_J_TRANSPARENT(l1, lbl)                           __endasm; \
    __asm cjne  a, BUS_PORT, l1                                 __endasm; \
    __asm ljmp  lbl                                             __endasm; \
    __asm l1:                                                   __endasm; \
    __asm
    
// Chroma-key test with ljmp to each result
#define CHROMA_LONG(l1, opaque, transparent)                        __endasm; \
    __asm   CHROMA_J_TRANSPARENT(l1, transparent)                   __endasm; \
    __asm   ljmp    opaque                                          __endasm; \
    __asm


/*
 * Update per-pixel BG0/BG1 accumulators.
 * We want to update only the X portion; this works by adding four,
 * which covers the common case, then doing a quick overflow check.
 * In the less likely event that we overflowed, go back and fix it.
 */

#define BG0_NEXT_PIXEL(l1, l2)                                  __endasm; \
    __asm mov   a, R_BG0_ADDR                                   __endasm; \
    __asm add   a, #4                                           __endasm; \
    __asm mov   R_BG0_ADDR, a                                   __endasm; \
    __asm anl   a, #0x1F                                        __endasm; \
    __asm jnz   l1                                              __endasm; \
    __asm mov   a, R_BG0_ADDR                                   __endasm; \
    __asm add   a, #(0x100 - 0x20)                              __endasm; \
    __asm mov   R_BG0_ADDR, a                                   __endasm; \
    __asm mov   _DPS, #0                                        __endasm; \
    __asm inc   dptr                                            __endasm; \
    __asm inc   dptr                                            __endasm; \
    __asm ASM_X_WRAP_CHECK(l2)                                  __endasm; \
    __asm l1:                                                   __endasm; \
    __asm

#define BG1_NEXT_PIXEL(l1, l2)                                  __endasm; \
    __asm mov   a, R_BG1_ADDR                                   __endasm; \
    __asm add   a, #4                                           __endasm; \
    __asm mov   R_BG1_ADDR, a                                   __endasm; \
    __asm anl   a, #0x1F                                        __endasm; \
    __asm jnz   l1                                              __endasm; \
    __asm BG1_NEXT_BIT()                                        __endasm; \
    __asm mov   a, R_BG1_ADDR                                   __endasm; \
    __asm add   a, #(0x100 - 0x20)                              __endasm; \
    __asm mov   R_BG1_ADDR, a                                   __endasm; \
    __asm BG1_JNB(l2)                                           __endasm; \
    __asm mov   _DPS, #1                                        __endasm; \
    __asm ASM_BG1_NEXT()                                        __endasm; \
    __asm l2:                                                   __endasm; \
    __asm BG1_UPDATE_BIT()                                      __endasm; \
    __asm l1:                                                   __endasm; \
    __asm

#endif
