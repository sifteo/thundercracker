/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "graphics_bg0.h"

/*
 * Background BG0, plus a BG1 "overlay" which is a smaller-than-screen-sized
 * tile grid that is allocated configurably using a separate bit vector.
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
 */

/*
 *   c: Current BG1 tile bit
 *  r0: Scratch
 *  r1: X wrap counter
 *  r3: BG0 addr low-byte
 *  r4: BG1 addr low-byte
 *  r5: Tiles / pixels remaining in draw loop
 *  r6: BG1 tile bitmap (next)
 *  r7: BG1 tile bitmap (current)
 */

/*
 * The workhorse of this mode is a set of unrolled state machines
 * which render 'r5' full tiles, on lines with BG0 and BG1 visible:
 *
 *    vm_bg0_bg1_tiles_fast_pN()
 *
 * Where 'N' is the panning delta from BG0 to BG1.
 *
 * We have ladders of contiguous states, for BG0-only and BG1-only.
 * Inline with the BG1 ladder are interleaved cases for
 * falling-through to BG0 pixels, as implemented by the
 * CHROMA_BG1_BG0() macro.
 *
 * Our state vector consists of layer (BG0/BG1) and X pixel. On entry,
 * we must already be set up for the first pixel of BG0.
 */

static uint8_t x_bg1_offset;            // Panning offset from BG0, multiplied by 4.
static uint8_t x_bg1_first_addr;        // Low address offset for first displayed tile
static uint8_t x_bg1_shift;             // Amount to shift bitmap by at the start of the line, plus one

static uint8_t y_bg1_addr_l;            // Low part of tile addresses, inc by 32 each line
static uint8_t y_bg1_word;              // Low part of bitmap address for this line
static uint16_t y_bg1_map;              // Map address for the first tile on this line

// Shift the next tile bit into C
#define BG1_NEXT_BIT(lbl)                                       __endasm; \
    __asm mov   a, r6                                           __endasm; \
    __asm clr   c                                               __endasm; \
    __asm rrc   a                                               __endasm; \
    __asm mov   r6, a                                           __endasm; \
    __asm mov   a, r7                                           __endasm; \
    __asm rrc   a                                               __endasm; \
    __asm mov   r7, a                                           __endasm; \
    __asm jc    lbl                                             __endasm; \
    __asm

// Load bitmap from globals. If 'a' is zero on exit, no bits are set.
#define BG1_LOAD_BITS()                                         __endasm; \
    __asm mov   _DPL, _y_bg1_word                               __endasm; \
    __asm mov   _DPH, #(_SYS_VA_BG1_BITMAP >> 8)                __endasm; \
    __asm movx  a, @dptr                                        __endasm; \
    __asm mov   r7, a                                           __endasm; \
    __asm inc   dptr                                            __endasm; \
    __asm movx  a, @dptr                                        __endasm; \
    __asm mov   r6, a                                           __endasm; \
    __asm orl   a, r7                                           __endasm; \
    __asm

#define BG0_BG1_LOAD_MAPS()                                     __endasm; \
    __asm mov   _DPL, _y_bg0_map                                __endasm; \
    __asm mov   _DPH, (_y_bg0_map+1)                            __endasm; \
    __asm mov   _DPL1, _y_bg1_map                               __endasm; \
    __asm mov   _DPH1, (_y_bg1_map+1)                           __endasm; \
    __asm

#define CHROMA_PREP()                                           __endasm; \
    __asm mov   a, #_SYS_CHROMA_KEY                             __endasm; \
    __asm

#define CHROMA_J_OPAQUE(lbl)                                    __endasm; \
    __asm cjne  a, BUS_PORT, lbl                                __endasm; \
    __asm

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

// Next BG0 tile (while in BG0 state)
#define ASM_BG0_NEXT(lbl)                                       __endasm; \
    __asm inc   dptr                                            __endasm; \
    __asm inc   dptr                                            __endasm; \
    __asm ADDR_FROM_DPTR(_DPL)                                  __endasm; \
    __asm mov   ADDR_PORT, r3                                   __endasm; \
    __asm ASM_X_WRAP_CHECK(lbl)                                 __endasm; \
    __asm

// Next BG0 tile (while in BG1 state)
#define ASM_BG0_NEXT_FROM_BG1(lbl)                              __endasm; \
    __asm dec   _DPS                                            __endasm; \
    __asm inc   dptr                                            __endasm; \
    __asm inc   dptr                                            __endasm; \
    __asm inc   _DPS                                            __endasm; \
    __asm ASM_X_WRAP_CHECK(lbl)                                 __endasm; \
    __asm

// State transition, BG0 pixel to BG1 pixel
#define STATE_BG0_TO_BG1(x)                                     __endasm; \
    __asm lcall _state_bg0_to_bg1_func                          __endasm; \
    __asm add   a, #((x) * 4)                                   __endasm; \
    __asm mov   ADDR_PORT, a                                    __endasm; \
    __asm CHROMA_PREP()                                         __endasm; \
    __asm
    

static void state_bg0_to_bg1_func(void) __naked {
    __asm
        inc    _DPS
        ADDR_FROM_DPTR(_DPL1)
        mov    a, r4
        ret
    __endasm ;
}       

// State transition, BG1 pixel to BG0 pixel, at nonzero X offset
#define STATE_BG1_TO_BG0(x)                                     __endasm; \
    __asm lcall _state_bg1_to_bg0_func                          __endasm; \
    __asm add   a, #((x) * 4)                                   __endasm; \
    __asm mov   ADDR_PORT, a                                    __endasm; \
    __asm
    
static void state_bg1_to_bg0_func(void) __naked {
    __asm
        dec    _DPS
        ADDR_FROM_DPTR(_DPL)
        mov    a, r3
        ret
    __endasm ;
}

// Overlaid pixel, BG1 over BG0
#define CHROMA_BG1_BG0(l1,l2,x0,x1)                             __endasm; \
    __asm CHROMA_J_OPAQUE(l1)                                   __endasm; \
    __asm   STATE_BG1_TO_BG0(x0)                                __endasm; \
    __asm   ASM_ADDR_INC4()                                     __endasm; \
    __asm   STATE_BG0_TO_BG1(x1)                                __endasm; \
    __asm   sjmp l2                                             __endasm; \
    __asm l1:                                                   __endasm; \
    __asm   ASM_ADDR_INC4()                                     __endasm; \
    __asm l2:                                                   __endasm; \
    __asm

static void vm_bg0_bg1_tiles_fast_pre(void) __naked
{
    /*
     * Common setup code for all of the vm_bg0_bg1_tiles_fast_pN functions.
     * This needs to be done after the computed jump, since there's no way
     * to do the jump without trashing a DPTR.
     */

    __asm

        BG0_BG1_LOAD_MAPS()                     ; Set up DPTR and DPTR1

        inc     dptr                            ; Skip first partial BG0 tile
        inc     dptr

        ADDR_FROM_DPTR(_DPL)                    ; Start out in BG0 state, at pixel 0
        mov     ADDR_PORT, r3

        ret
    __endasm ;
}

static void vm_bg0_bg1_tiles_fast_p0(void) __naked
{
    __asm
        lcall   _vm_bg0_bg1_tiles_fast_pre

10$:
        BG1_NEXT_BIT(1$)
        lcall   _addr_inc32

11$:    ASM_BG0_NEXT(9$)
        djnz    r5, 10$
        ret

1$:     STATE_BG0_TO_BG1(0)                     ; Entry BG0->BG1
        CHROMA_BG1_BG0(30$, 40$, 0,0)           ; Keyed BG1 pixel 0
        CHROMA_BG1_BG0(31$, 41$, 1,1)           ; Keyed BG1 pixel 1
        CHROMA_BG1_BG0(32$, 42$, 2,2)           ; Keyed BG1 pixel 2
        CHROMA_BG1_BG0(33$, 43$, 3,3)           ; Keyed BG1 pixel 3
        CHROMA_BG1_BG0(34$, 44$, 4,4)           ; Keyed BG1 pixel 4
        CHROMA_BG1_BG0(35$, 45$, 5,5)           ; Keyed BG1 pixel 5
        CHROMA_BG1_BG0(36$, 46$, 6,6)           ; Keyed BG1 pixel 6
        CHROMA_BG1_BG0(37$, 47$, 7,7)           ; Keyed BG1 pixel 7
        ASM_DPTR_INC2()                         ; Next BG1 Tile
        dec     _DPS                            ;   BG1 -> BG0
        ljmp    11$                             ;   Return to BG0 ladder

    __endasm ;
}

static void vm_bg0_bg1_tiles_fast_p1(void) __naked
{
    __asm
        lcall   _vm_bg0_bg1_tiles_fast_pre

10$:
        ASM_ADDR_INC4()                         ; BG0 Pixel 0
12$:    BG1_NEXT_BIT(1$)
        lcall   _addr_inc28                     ; BG0 Pixel 1-7

11$:    ASM_BG0_NEXT(9$)
        djnz    r5, 10$
        ret

1$:     STATE_BG0_TO_BG1(0)                     ; Entry BG0->BG1
        CHROMA_BG1_BG0(30$, 40$, 1,0)           ; Keyed BG1 pixel 0
        CHROMA_BG1_BG0(31$, 41$, 2,1)           ; Keyed BG1 pixel 1
        CHROMA_BG1_BG0(32$, 42$, 3,2)           ; Keyed BG1 pixel 2
        CHROMA_BG1_BG0(33$, 43$, 4,3)           ; Keyed BG1 pixel 3
        CHROMA_BG1_BG0(34$, 44$, 5,4)           ; Keyed BG1 pixel 4
        CHROMA_BG1_BG0(35$, 45$, 6,5)           ; Keyed BG1 pixel 5
        CHROMA_BG1_BG0(36$, 46$, 7,6)           ; Keyed BG1 pixel 6
        ASM_BG0_NEXT_FROM_BG1(8$)               ; Next BG0 tile
        CHROMA_BG1_BG0(37$, 47$, 0,7)           ; Keyed BG1 pixel 7
        ASM_DPTR_INC2()                         ; Next BG1 Tile
        djnz    r5, 13$
        ret

13$:    STATE_BG1_TO_BG0(1)                     ; Return to BG0
        ljmp    12$

    __endasm ;
}

static void vm_bg0_bg1_tiles_fast_p2(void) __naked
{
    __asm
        lcall   _vm_bg0_bg1_tiles_fast_pre

10$:
        lcall   _addr_inc8                      ; BG0 Pixel 0-1
12$:    BG1_NEXT_BIT(1$)
        lcall   _addr_inc24                     ; BG0 Pixel 2-7

11$:    ASM_BG0_NEXT(9$)
        djnz    r5, 10$
        ret

1$:     STATE_BG0_TO_BG1(0)                     ; Entry BG0->BG1
        CHROMA_BG1_BG0(30$, 40$, 2,0)           ; Keyed BG1 pixel 0
        CHROMA_BG1_BG0(31$, 41$, 3,1)           ; Keyed BG1 pixel 1
        CHROMA_BG1_BG0(32$, 42$, 4,2)           ; Keyed BG1 pixel 2
        CHROMA_BG1_BG0(33$, 43$, 5,3)           ; Keyed BG1 pixel 3
        CHROMA_BG1_BG0(34$, 44$, 6,4)           ; Keyed BG1 pixel 4
        CHROMA_BG1_BG0(35$, 45$, 7,5)           ; Keyed BG1 pixel 5
        ASM_BG0_NEXT_FROM_BG1(8$)               ; Next BG0 tile
        CHROMA_BG1_BG0(36$, 46$, 0,6)           ; Keyed BG1 pixel 6
        CHROMA_BG1_BG0(37$, 47$, 1,7)           ; Keyed BG1 pixel 7
        ASM_DPTR_INC2()                         ; Next BG1 Tile
        djnz    r5, 13$
        ret

13$:    STATE_BG1_TO_BG0(2)                     ; Return to BG0
        ljmp    12$

    __endasm ;
}

static void vm_bg0_bg1_tiles_fast_p3(void) __naked
{
    __asm
        lcall   _vm_bg0_bg1_tiles_fast_pre

10$:
        lcall   _addr_inc12                     ; BG0 Pixel 0-2
12$:    BG1_NEXT_BIT(1$)
        lcall   _addr_inc20                     ; BG0 Pixel 3-7

11$:    ASM_BG0_NEXT(9$)
        djnz    r5, 10$
        ret

1$:     STATE_BG0_TO_BG1(0)                     ; Entry BG0->BG1
        CHROMA_BG1_BG0(30$, 40$, 3,0)           ; Keyed BG1 pixel 0
        CHROMA_BG1_BG0(31$, 41$, 4,1)           ; Keyed BG1 pixel 1
        CHROMA_BG1_BG0(32$, 42$, 5,2)           ; Keyed BG1 pixel 2
        CHROMA_BG1_BG0(33$, 43$, 6,3)           ; Keyed BG1 pixel 3
        CHROMA_BG1_BG0(34$, 44$, 7,4)           ; Keyed BG1 pixel 4
        ASM_BG0_NEXT_FROM_BG1(8$)               ; Next BG0 tile
        CHROMA_BG1_BG0(35$, 45$, 0,5)           ; Keyed BG1 pixel 5
        CHROMA_BG1_BG0(36$, 46$, 1,6)           ; Keyed BG1 pixel 6
        CHROMA_BG1_BG0(37$, 47$, 2,7)           ; Keyed BG1 pixel 7
        ASM_DPTR_INC2()                         ; Next BG1 Tile
        djnz    r5, 13$
        ret

13$:    STATE_BG1_TO_BG0(3)                     ; Return to BG0
        ljmp    12$

    __endasm ;
}

static void vm_bg0_bg1_tiles_fast_p4(void) __naked
{
    __asm
        lcall   _vm_bg0_bg1_tiles_fast_pre

10$:
        lcall   _addr_inc16                     ; BG0 Pixel 0-3
12$:    BG1_NEXT_BIT(1$)
        lcall   _addr_inc16                     ; BG0 Pixel 4-7

11$:    ASM_BG0_NEXT(9$)
        djnz    r5, 10$
        ret

1$:     STATE_BG0_TO_BG1(0)                     ; Entry BG0->BG1
        CHROMA_BG1_BG0(30$, 40$, 4,0)           ; Keyed BG1 pixel 0
        CHROMA_BG1_BG0(31$, 41$, 5,1)           ; Keyed BG1 pixel 1
        CHROMA_BG1_BG0(32$, 42$, 6,2)           ; Keyed BG1 pixel 2
        CHROMA_BG1_BG0(33$, 43$, 7,3)           ; Keyed BG1 pixel 3
        ASM_BG0_NEXT_FROM_BG1(8$)               ; Next BG0 tile
        CHROMA_BG1_BG0(34$, 44$, 0,4)           ; Keyed BG1 pixel 4
        CHROMA_BG1_BG0(35$, 45$, 1,5)           ; Keyed BG1 pixel 5
        CHROMA_BG1_BG0(36$, 46$, 2,6)           ; Keyed BG1 pixel 6
        CHROMA_BG1_BG0(37$, 47$, 3,7)           ; Keyed BG1 pixel 7
        ASM_DPTR_INC2()                         ; Next BG1 Tile
        djnz    r5, 13$
        ret

13$:    STATE_BG1_TO_BG0(4)                     ; Return to BG0
        ljmp    12$

    __endasm ;
}

static void vm_bg0_bg1_tiles_fast_p5(void) __naked
{
    __asm
        lcall   _vm_bg0_bg1_tiles_fast_pre

10$:
        lcall   _addr_inc20                     ; BG0 Pixel 0-4
12$:    BG1_NEXT_BIT(1$)
        lcall   _addr_inc12                     ; BG0 Pixel 5-7

11$:    ASM_BG0_NEXT(9$)
        djnz    r5, 10$
        ret

1$:     STATE_BG0_TO_BG1(0)                     ; Entry BG0->BG1
        CHROMA_BG1_BG0(30$, 40$, 5,0)           ; Keyed BG1 pixel 0
        CHROMA_BG1_BG0(31$, 41$, 6,1)           ; Keyed BG1 pixel 1
        CHROMA_BG1_BG0(32$, 42$, 7,2)           ; Keyed BG1 pixel 2
        ASM_BG0_NEXT_FROM_BG1(8$)               ; Next BG0 tile
        CHROMA_BG1_BG0(33$, 43$, 0,3)           ; Keyed BG1 pixel 3
        CHROMA_BG1_BG0(34$, 44$, 1,4)           ; Keyed BG1 pixel 4
        CHROMA_BG1_BG0(35$, 45$, 2,5)           ; Keyed BG1 pixel 5
        CHROMA_BG1_BG0(36$, 46$, 3,6)           ; Keyed BG1 pixel 6
        CHROMA_BG1_BG0(37$, 47$, 4,7)           ; Keyed BG1 pixel 7
        ASM_DPTR_INC2()                         ; Next BG1 Tile
        djnz    r5, 13$
        ret

13$:    STATE_BG1_TO_BG0(5)                     ; Return to BG0
        ljmp    12$

    __endasm ;
}

static void vm_bg0_bg1_tiles_fast_p6(void) __naked
{
    __asm
        lcall   _vm_bg0_bg1_tiles_fast_pre

10$:
        lcall   _addr_inc24                     ; BG0 Pixel 0-5
12$:    BG1_NEXT_BIT(1$)
        lcall   _addr_inc8                      ; BG0 Pixel 6-7

11$:    ASM_BG0_NEXT(9$)
        djnz    r5, 10$
        ret

1$:     STATE_BG0_TO_BG1(0)                     ; Entry BG0->BG1
        CHROMA_BG1_BG0(30$, 40$, 6,0)           ; Keyed BG1 pixel 0
        CHROMA_BG1_BG0(31$, 41$, 7,1)           ; Keyed BG1 pixel 1
        ASM_BG0_NEXT_FROM_BG1(8$)               ; Next BG0 tile
        CHROMA_BG1_BG0(32$, 42$, 0,2)           ; Keyed BG1 pixel 2
        CHROMA_BG1_BG0(33$, 43$, 1,3)           ; Keyed BG1 pixel 3
        CHROMA_BG1_BG0(34$, 44$, 2,4)           ; Keyed BG1 pixel 4
        CHROMA_BG1_BG0(35$, 45$, 3,5)           ; Keyed BG1 pixel 5
        CHROMA_BG1_BG0(36$, 46$, 4,6)           ; Keyed BG1 pixel 6
        CHROMA_BG1_BG0(37$, 47$, 5,7)           ; Keyed BG1 pixel 7
        ASM_DPTR_INC2()                         ; Next BG1 Tile
        djnz    r5, 13$
        ret

13$:    STATE_BG1_TO_BG0(6)                     ; Return to BG0
        ljmp    12$

    __endasm ;
}

static void vm_bg0_bg1_tiles_fast_p7(void) __naked
{
    __asm
        lcall   _vm_bg0_bg1_tiles_fast_pre

10$:
        lcall   _addr_inc28                     ; BG0 Pixel 0-6
12$:    BG1_NEXT_BIT(1$)
        ASM_ADDR_INC4()                         ; BG0 Pixel 7

11$:    ASM_BG0_NEXT(9$)
        djnz    r5, 10$
        ret

1$:     STATE_BG0_TO_BG1(0)                     ; Entry BG0->BG1
        CHROMA_BG1_BG0(30$, 40$, 7,0)           ; Keyed BG1 pixel 0
        ASM_BG0_NEXT_FROM_BG1(8$)               ; Next BG0 tile
        CHROMA_BG1_BG0(31$, 41$, 0,1)           ; Keyed BG1 pixel 1
        CHROMA_BG1_BG0(32$, 42$, 1,2)           ; Keyed BG1 pixel 2
        CHROMA_BG1_BG0(33$, 43$, 2,3)           ; Keyed BG1 pixel 3
        CHROMA_BG1_BG0(34$, 44$, 3,4)           ; Keyed BG1 pixel 4
        CHROMA_BG1_BG0(35$, 45$, 4,5)           ; Keyed BG1 pixel 5
        CHROMA_BG1_BG0(36$, 46$, 5,6)           ; Keyed BG1 pixel 6
        CHROMA_BG1_BG0(37$, 47$, 6,7)           ; Keyed BG1 pixel 7
        ASM_DPTR_INC2()                         ; Next BG1 Tile
        djnz    r5, 13$
        ret

13$:    STATE_BG1_TO_BG0(7)                     ; Return to BG0
        ljmp    12$

    __endasm ;
}

static void vm_bg0_bg1_tiles_fast(void) __naked
{
    /*
     * Render 'r5' tiles from bg0, quickly, with bg1 overlaid
     * at the proper panning offset from x_bg1_offset.
     */

    __asm
        mov     a, _x_bg1_offset
        mov     dptr, #1$
        jmp     @a+dptr

	; These redundant labels are required by the binary translator!

1$:     ljmp    _vm_bg0_bg1_tiles_fast_p0
        nop
2$:     ljmp    _vm_bg0_bg1_tiles_fast_p1
        nop
3$:     ljmp    _vm_bg0_bg1_tiles_fast_p2
        nop
4$:     ljmp    _vm_bg0_bg1_tiles_fast_p3
        nop
5$:     ljmp    _vm_bg0_bg1_tiles_fast_p4
        nop
6$:     ljmp    _vm_bg0_bg1_tiles_fast_p5
        nop
7$:     ljmp    _vm_bg0_bg1_tiles_fast_p6
        nop
8$:     ljmp    _vm_bg0_bg1_tiles_fast_p7
    __endasm ;
}

static void vm_bg0_bg1_line(void)
{
    /*
     * Scanline renderer: BG0 and BG1 visible.
     */


     /*
      * Load BG1 bitmap for this line. If no BG1 tiles are visible, we can
      * tail-call to the BG0-only scanline renderer.
      */

    __asm
        BG1_LOAD_BITS()
        jnz     1$
        ljmp    _vm_bg0_line            ; No BG1 bits on this line
1$:     
    __endasm ;

    /*
     * We keep the wrap counter stored in a register, for fast x-wrap checks.
     */

    __asm
        mov     r1, _x_bg0_wrap
    __endasm ;

    /*
     * Segment the line into a fast full-tile burst, and up to two slower partial tiles.
     */

    __asm

        ; First tile, may be skipping up to 7 pixels from the beginning

        mov     r5, _x_bg0_first_w
2$:     inc     ADDR_PORT
        inc     ADDR_PORT
        inc     ADDR_PORT
        inc     ADDR_PORT
        djnz    r5, 2$

        ; Always have a run of 15 full tiles

        mov     r3, _y_bg0_addr_l
        mov     r4, _y_bg1_addr_l
        mov     r5, #15
        lcall   _vm_bg0_bg1_tiles_fast

        ; May have a final partial tile

        mov     a, _x_bg0_last_w
        jz      3$
        mov     r5, a
4$:     inc     ADDR_PORT
        inc     ADDR_PORT
        inc     ADDR_PORT
        inc     ADDR_PORT
        djnz    r5, 4$
3$:

    __endasm ;

    CTRL_PORT = CTRL_IDLE;
}

static void vm_bg1_setup(void)
{
    /*
     * Once-per-frame setup for BG1.
     * BG0 must have already been set up for this frame.
     */
     
    uint8_t pan_x, pan_y;
    uint8_t tile_pan_x, tile_pan_y;

    cli();
    pan_x = vram.bg1_x;
    pan_y = vram.bg1_y;
    sti();

    tile_pan_x = pan_x >> 3;
    tile_pan_y = pan_y >> 3;

    y_bg1_addr_l = pan_y << 5;
    y_bg1_word = (tile_pan_y << 1) + (_SYS_VA_BG1_BITMAP & 0xFF);

    x_bg1_offset = ((x_bg0_last_w - pan_x) & 7) << 2;
    x_bg1_shift = tile_pan_x + 1;
    x_bg1_first_addr = (pan_x << 2) & 0x1C;

    /*
     * XXX: To find the initial value of y_bg1_map, we have to scan
     * for '1' bits in the bitmap.
     *
     * We need to set both y_bg1_map and DPTR1, since we don't know
     * whether the first line will end up loading DPTR1 or not. If
     * the first line has no BG1 tiles, we still need to have the
     * right pointer resident in DPTR1, since vm_bg1_next() will
     * still try to save it.
     */

    y_bg1_map = _SYS_VA_BG1_TILES;
    DPH1 = _SYS_VA_BG1_TILES >> 8;
    DPL1 = _SYS_VA_BG1_TILES & 0xFF;
}

static void vm_bg1_next(void)
{
    /*
     * Advance BG1 state to the next line
     */

    y_bg1_addr_l += 32;
    if (!y_bg1_addr_l) {
        y_bg1_word += 2;

        /*
         * If we're advancing, save the DPTR1 advancement that we performed
         * during this line. Otherwise, it gets discarded at the next line.
         */
        __asm
            mov   _y_bg1_map, _DPL1
            mov   (_y_bg1_map+1), _DPH1
        __endasm ;
    }
}

void vm_bg0_bg1(void)
{
    uint8_t y = vram.num_lines;

    lcd_begin_frame();
    vm_bg0_setup();
    vm_bg1_setup();
    
    do {
        vm_bg0_bg1_line();

        vm_bg0_next();
        vm_bg1_next();
    } while (--y);    

    lcd_end_frame();
    MODE_RETURN();
}
