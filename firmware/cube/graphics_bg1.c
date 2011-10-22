/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "graphics_bg1.h"
#include "hardware.h"
#include "radio.h"

uint8_t x_bg1_offset_x4, x_bg1_shift;
__bit x_bg1_rshift, x_bg1_lshift;

uint8_t y_bg1_addr_l, y_bg1_bit_index;
uint16_t y_bg1_map;
__bit y_bg1_empty;


void vm_bg0_bg1(void)
{
    uint8_t y = vram.num_lines;

    lcd_begin_frame();
    vm_bg0_setup();
    vm_bg1_setup();

    do {
        if (y_bg1_empty)
            vm_bg0_line();
        else
            vm_bg0_bg1_line();

        vm_bg0_next();
        vm_bg1_next();
    } while (--y);    

    lcd_end_frame();
}


static void vm_bg1_begin_line(void) __naked
{
    /*
     * Using the current values of y_bg1_bit_index and y_bg1_shift,
     * set up the MDU with the 32-bit bitmap to use for this scanline.
     *
     * This needs to happen at least 18 clock cycles prior to the beginning
     * of the next scanline, so that the MDU shift has time to happen.
     *
     * If we're shifting right, we need to do this one step at a time and
     * update y_bg1_map as we go.
     */
    
    __asm
        ; Test whether we are past the edge of the bitmap in either direction
        
        mov     a, _y_bg1_bit_index
        anl     a, #0xF0
        jnz     1$
                
        ; Bitmap is not out-of-range. Load it,
        ; check whether it is zero, and pad to 32 bits.
        
        mov     a, _y_bg1_bit_index
        rl      a                           ; Index -> byte address
        add     a, #(_SYS_VA_BG1_BITMAP & 0xFF)
        mov     dpl, a
        mov     dph, #(_SYS_VA_BG1_BITMAP >> 8)
        
        movx    a, @dptr
        mov     _MD0, a
        inc     dptr                        ; Low byte is zero.. see if the high byte is too.
        jnz     2$                          ; Definitely not zero
        movx    a, @dptr
        jz      1$                          ; Yep. Exit via setting y_bg1_empty.
2$:     movx    a, @dptr                    ; Nonzero. Finish storing to MD1.
        mov     _MD1, a
        mov     _MD2, #0
        mov     _MD3, #0

        clr     _y_bg1_empty                ; Remember that this line is non-empty.
        mov     _DPL1, _y_bg1_map           ; Load BG1 tile address for this line
        mov     _DPH1, (_y_bg1_map+1)
        
        ; Now perform the shift, if we need to.
        
        jnb     _x_bg1_rshift, 3$           ; Right shift, one step at a time
        mov     r0, _x_bg1_shift            ; Loop iterator
        inc     _DPS                        ; Select DPTR1, for fast increments
5$:
        mov     a, _MD0                     ; Examine the LSB
        mov     _MD0, a                     ;    dummy write to reset the MDU state
        mov     _ARCON, #0x21               ;    Begin right shift by one
        jnb     acc.0, 6$                   ;    continue if we are about to skip a zero
        inc     dptr                        ;    skip a tile if we are skipping a zero
        inc     dptr
6$:     djnz    r0, 5$                      ; Next bit...
        dec     _DPS                        ; Back to original DPTR
        anl     _DPH1, #3                   ; Mask DPTR to keep it inside of xram
        
3$:     jnb     _x_bg1_lshift, 4$           ; Left shift, asynchronously using ARCON
        mov     _ARCON, _x_bg1_shift
        
4$:     ret                                 ; Normal exit
        
        ; No bits set? Instead of writing to the MDU, just set y_bg1_empty.

1$:     setb    _y_bg1_empty
        ret

    __endasm ;
}

static uint8_t bitcount16_x2(uint16_t x) __naked
{
    /*
     * Return 2x the number of '1' bits in x.
     * Returns to both dpl (for C) and acc (for asm)
     */

    static const __code uint8_t lut[16] = {        
        /* 0000 */ 0,  /* 0001 */ 1,  /* 0010 */ 1,  /* 0011 */ 2,
        /* 0100 */ 1,  /* 0101 */ 2,  /* 0110 */ 2,  /* 0111 */ 3,
        /* 1000 */ 1,  /* 1001 */ 2,  /* 1010 */ 2,  /* 1011 */ 3,
        /* 1100 */ 2,  /* 1101 */ 3,  /* 1110 */ 3,  /* 1111 */ 4,
    };

    x = x;
    
    __asm
        push    ar0
        push    ar1
        push    ar2
        
        mov     r0, dpl
        mov	    r1, dph
        mov     dptr, #_bitcount16_x2_lut_1_1

        mov	    a, #0x0F
        anl     a, r0
        movc    a, @a+dptr
        mov     r2, a

        mov	    a, #0xF0
        anl     a, r0
        swap    a
        movc    a, @a+dptr
        add     a, r2
        mov     r2, a

        mov	    a, #0x0F
        anl     a, r1
        movc    a, @a+dptr
        add     a, r2
        mov     r2, a

        mov	    a, #0xF0
        anl     a, r1
        swap    a
        movc    a, @a+dptr
        add     a, r2
        
        rl      a
        mov     dpl, a

        pop     ar2
        pop     ar1
        pop     ar0
        ret
    __endasm ;
}   

void vm_bg1_setup(void)
{
    /*
     * Once-per-frame setup for BG1.
     * BG0 must have already been set up for this frame.
     */
     
    {
        uint8_t pan_x, pan_y;
        uint8_t tile_pan_x, tile_pan_y;

        radio_critical_section({
            pan_x = vram.bg1_x;
            pan_y = vram.bg1_y;
        });
        
        tile_pan_x = pan_x >> 3;
        tile_pan_y = pan_y >> 3;

        y_bg1_addr_l = pan_y << 5;
        y_bg1_bit_index = tile_pan_y;
        
        x_bg1_offset_x4 = ((x_bg0_last_w - pan_x) & 7) << 2;
            
        x_bg1_lshift = 0;
        x_bg1_rshift = 0;
    
        if (tile_pan_x & 0xF0) {
            /*
            * We're starting at a point outside the valid part of the BG1 bitmap.
            * Set up x_bg1_shift to shift left, so that we'll eventually see the
            * valid bits once we wrap around back to zero.
            *
            * Since we aren't skipping over any '1' bits, we don't need to do any
            * corresponding adjustment of y_bg1_map at each scanline. So, we can
            * do this shift quickly using the MDU.
            */

            x_bg1_shift = 0x20 - tile_pan_x;
            x_bg1_lshift = 1;
    
        } else if (tile_pan_x) {
            /*
            * Starting inside the BG1 bitmap. Shift right, according to the number
            * of tiles we've passed. Since we may be skipping '1' bits here, we need
            * to do the shift one step at a time.
            */

            x_bg1_shift = tile_pan_x;
            x_bg1_rshift = 1;
        }
    }
    
    /*
     * Calculate the address of the first tile on the screen. This
     * kind of sucks, since we have to iterate over all of those bitmap
     * words we're skipping. We handle the horizontal portion in
     * vm_bg1_begin_line(), but here we need to count the number of '1'
     * bits in any word that we're skipping.
     */
     
    y_bg1_map = _SYS_VA_BG1_TILES;
    
    if (y_bg1_bit_index < 0x10) {
        uint8_t i = 0;
        while (i != y_bg1_bit_index) {
            y_bg1_map += bitcount16_x2(vram.bg1_bitmap[i]);
            i++;
        }
        y_bg1_map &= 0x3FF;
    }

    // Tail call to prepare our state for the first scanline
    vm_bg1_begin_line();
}

static void vm_bg1_next(void)
{
    /*
     * Advance BG1 state to the next line
     */

    y_bg1_addr_l += 32;
    if (!y_bg1_addr_l) {
        /*
         * Next bitmap word
         */
        y_bg1_bit_index = (y_bg1_bit_index + 1) & 0x1F;

        /*
         * If we're advancing, save the DPTR1 advancement that we performed
         * during this line. Otherwise, it gets discarded at the next line.
         *
         * If we have any 1 bits remaining in the MDU, those are tiles we haven't
         * advanced through yet. Adjust y_bg1_map accordingly. At this point we
         * can be guaranteed that the only ones are in the low 16 bits.
         */
        if (!y_bg1_empty) {
            __asm
                mov     dpl, _MD0
                mov     dph, _MD1
                lcall   _bitcount16_x2

                add     a, _DPL1
                mov     _y_bg1_map, a
                clr     a
                addc    a, _DPH1
                mov     (_y_bg1_map+1), a
            __endasm ;
        }
    }
    
    // Tail call to prepare the next line's bitmap
    vm_bg1_begin_line();
}


void vm_bg0_bg1_line(void)
{
    /*
     * XXX: Very slow unoptimized implementation. Using this to develop tests...
     */

    uint8_t x_bg0_addr = x_bg0_first_addr;
    uint8_t x_bg1_addr = (vram.bg1_x << 2) & 0x1F;
    uint8_t bg0_wrap = x_bg0_wrap;
    uint8_t x = 128;
    
    DPTR = y_bg0_map;
    
    do {
        if (MD0 & 1) {
            // BG1 chroma-keyed over BG0

            __asm
                inc     _DPS
                ADDR_FROM_DPTR(_DPL1)
                dec     _DPS
            __endasm ;
            ADDR_PORT = y_bg1_addr_l + x_bg1_addr;
            
            if (BUS_PORT == _SYS_CHROMA_KEY) {
                // BG0 pixel
                __asm ADDR_FROM_DPTR(_DPL) __endasm;
                ADDR_PORT = y_bg0_addr_l + x_bg0_addr;
            }

            ADDR_INC4();

        } else {
            // BG0 pixel
            __asm ADDR_FROM_DPTR(_DPL) __endasm;
            ADDR_PORT = y_bg0_addr_l + x_bg0_addr;
            ADDR_INC4();
        }
        
        if (!(x_bg0_addr = (x_bg0_addr + 4) & 0x1F)) {
            DPTR_INC2();
            BG0_WRAP_CHECK();
        }
        
        if (!(x_bg1_addr = (x_bg1_addr + 4) & 0x1F)) {
            if (MD0 & 1) {
                __asm
                    inc     _DPS
                    inc     dptr
                    inc     dptr
                    dec     _DPS
                    anl     _DPH1, #3
                __endasm ;
            }
            MD0 = MD0;  // Dummy write to reset MDU
            ARCON = 0x21;
        }
        
    } while (--x);
}



/*************************** TO REWORK ********************************************************/
 
#if 0


/*
 * Register allocation, during our scanline renderer:
 *
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
static uint8_t y_bg1_bit_addr;          // Index into bitmap array
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
     * We keep the wrap counter stored in a register, for fast x-wrap checks.
     */

    __asm
        mov     r1, _x_bg0_wrap
    __endasm ;

    /*
     * Segment the line into a fast full-tile burst, and up to two slower partial tiles.
     */


        inc     dptr                            ; Skip first partial BG0 tile
        inc     dptr

        ADDR_FROM_DPTR(_DPL)                    ; Start out in BG0 state, at pixel 0
        
            // First partial or full tile
    ADDR_FROM_DPTR_INC();
    BG0_WRAP_CHECK();
    ADDR_PORT = y_bg0_addr_l + x_bg0_first_addr;
    PIXEL_BURST(x_bg0_first_w);

    __asm

        ; First tile, may be skipping up to 7 pixels from the beginning

        BG0_BG1_LOAD_MAPS()
        ASM_ADDR_FROM_DPTR_INC()
        BG0_WRAP_CHECK
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



#endif

