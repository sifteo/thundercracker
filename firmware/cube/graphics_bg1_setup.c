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

uint8_t x_bg1_shift, x_bg1_first_addr, x_bg1_last_addr;
__bit x_bg1_rshift, x_bg1_lshift;
__bit x_bg1_offset_bit0, x_bg1_offset_bit1, x_bg1_offset_bit2;

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
     * If we're shifting right, we need to do this one step at a time and
     * update y_bg1_map as we go.
     *
     * This programs DPTR1 and the MDU with data that needs to stay valid
     * by the time we reach the scanline renderer!
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
        
        ; Now perform the shift, if we need to.
        ;
        ; This is a bit convoluted, since the timing requirements differ
        ; depending on whether we are shifting right (using our loop) or left
        ; (with the MDU), so e.g. we reorder our load of DPTR1.
        
        jnb     _x_bg1_rshift, 3$           ; Right shift, one step at a time
        
        mov     _DPL1, _y_bg1_map           ; Load BG1 tile address _before_ shifting
        mov     _DPH1, (_y_bg1_map+1)
        
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
        
        sjmp    8$
        
3$:     jnb     _x_bg1_lshift, 4$           ; Left shift, asynchronously using ARCON
        mov     _ARCON, _x_bg1_shift  

        nop                                 ; Padding, see below.
        nop     
4$:       
        mov     _DPL1, _y_bg1_map           ; Load BG1 tile address while we wait
        mov     _DPH1, (_y_bg1_map+1)
8$:

        ; We need the results of the shift. Do some other work while we wait.
        ; The maximum shift is 16 bits, or 11 clock cycles of MDU activity.
        ; The code above has been padded so that this takes long enough in
        ; the worst case.
                
        clr     _y_bg1_empty                ; For now, remember that this line has BG1 tiles
        
        ; It is to our benefit to be really sure that there are BG1 tiles visible
        ; before exiting with y_bg1_empty cleared. If we can do a more thorough test
        ; here and avoid using the BG1 scanline renderer, that is a big win. So,
        ; we can look at the low 17 bits of the MDU. 17 is the maximum number of
        ; whole or partial tiles that may be visible. If none of these bits are set,
        ; we declare the line empty.
        
        mov     a, _MD0
        jnz     7$
        mov     a, _MD1
        jnz     7$
        mov     a, _MD2
        anl     a, #1
        jnz     7$
        
1$:     setb    _y_bg1_empty                ; No BG1 tiles on this scanline
7$:     ret

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
        mov         r1, dph
        mov     dptr, #_bitcount16_x2_lut_1_1

        mov         a, #0x0F
        anl     a, r0
        movc    a, @a+dptr
        mov     r2, a

        mov         a, #0xF0
        anl     a, r0
        swap    a
        movc    a, @a+dptr
        add     a, r2
        mov     r2, a

        mov         a, #0x0F
        anl     a, r1
        movc    a, @a+dptr
        add     a, r2
        mov     r2, a

        mov         a, #0xF0
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
        uint8_t x_bg1_offset;

        radio_critical_section({
            pan_x = vram.bg1_x;
            pan_y = vram.bg1_y;
        });
        
        tile_pan_x = pan_x >> 3;
        tile_pan_y = pan_y >> 3;

        y_bg1_addr_l = pan_y << 5;
        y_bg1_bit_index = tile_pan_y;
        
        x_bg1_offset = (x_bg0_last_w - pan_x) & 7;
        x_bg1_offset_bit0 = x_bg1_offset & 1;
        x_bg1_offset_bit1 = x_bg1_offset & 2;
        x_bg1_offset_bit2 = x_bg1_offset & 4;
        
        x_bg1_first_addr = (pan_x << 2) & 0x1C;
        x_bg1_last_addr = (8 - x_bg1_offset) << 2;
            
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
