/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "graphics_sprite.h"

struct x_sprite_t x_spr[_SYS_SPRITES_PER_LINE];
uint8_t y_spr_line, y_spr_line_limit, y_spr_active;


void vm_bg0_spr_bg1(void)
{
    lcd_begin_frame();
    vm_bg0_setup();
    vm_bg1_setup();
    vm_spr_setup();

    do {
        if (y_spr_active) {
            if (y_bg1_empty)
                vm_bg0_spr_line();
            else
                vm_bg0_spr_bg1_line();
        } else {
            if (y_bg1_empty)
                vm_bg0_line();
            else
                vm_bg0_bg1_line();
        }
                
        vm_bg0_next();
        vm_bg1_next();
        vm_spr_next();

    } while (y_spr_line != y_spr_line_limit);
    
    lcd_end_frame();
}

void vm_spr_setup()
{
    y_spr_line = 0xFF;    // Will wrap to zero in vm_spr_next()
    y_spr_line_limit = vram.num_lines;
    
    vm_spr_next();        // Tail call to set up the first line
}

void vm_spr_next()
{
    /*
     * Iterate over the sprites in VRAM.
     *
     * Do the Y test on each sprite. For each sprite that passes the
     * Y test, set the corresponding bit in y_spr_active and init
     * the X parameters in x_spr.
     *
     * dptr: Source address, in _SYS_VA_SPR
     *   r0: Dest address, in _x_spr
     *   r1: tile index adjustment
     *   r2: Loop counter
     *   r3: Copy of _y_spr_line
     *   r4: mask_y
     *   r5: mask_x
     *   r6: Sprite Y offset
     *   r7: pos_x 
     *
     * In general, we'd like _SYSSpriteInfo to be laid out in the same
     * order that it's most convenient to read here; i.e. all Y params
     * first, then X, then tile. But it's even more important that we keep
     * X/Y values packed into the same word, so they can be atomically
     * updated by our radio protocol.
     *
     * The end result of this loop is a tightly packed array of X-coordinate
     * information for only those sprites that are visible on the current
     * scanline.
     *
     * The particular encoding we use for sprite locations is kind of cool.
     * The width/height are powers of two, and the positions are arbitrary
     * signed 8-bit ints. They are all stored negated. The arithmetic works
     * out such that the negated width/height becomes a bit mask. If you add
     * the negated position to the current raster position, you get the pixel
     * offset from the beginning of the sprite. This isn't surprising, since 
     * really you just subtracted the sprite origin from the raster position.
     *
     * The cool part: AND that with the mask. The negated mask is the same thing
     * as ~(width-1), or the inverse of a bitmask that has log2(width) bits set.
     * So, if you AND this with the adjusted position and you don't get zero,
     * that tells you that the position is outside the sprite. This is a little
     * faster than doing a normal inequality test.
     *
     * The other common test we'll perform: We want to know, for the next N pixels,
     * will the sprite show up anywhere in that region. We use this here to
     * test whether the sprite is on-screen horizontally at all, and we can use it
     * in our inner loops to test whether a sprite shows up within a particular
     * tile.
     *
     * For this test, we can start with the same mask test. That will tell us if
     * we're already in the sprite. If and only if we aren't, we can see if it's
     * coming up soon by adding N to the adjusted sprite position, and checking
     * for overflow.
     */

    __asm
        inc     _y_spr_line
        mov     _y_spr_active, #0
        mov     dptr, #_SYS_VA_SPR
        mov     r0, #_x_spr
        mov     r2, #_SYS_VRAM_SPRITES
        mov     r3, _y_spr_line

1$:
        ; Y tests

        movx    a, @dptr    ; mask_y
        jz      16$         ;   Sprite disabled? Skip 6 bytes.
        inc     dptr
        mov     r4, a
        
        movx    a, @dptr    ; mask_x
        mov     r5, a       ;   Save for later
        inc     dptr

        movx    a, @dptr    ; pos_y
        add     a, r3       ;   Add to current line
        mov     r6, a       ;   Save Y offset
        anl     a, r4       ;   Apply mask
        jnz     14$         ;   Not within Y bounds? Skip 4 bytes.
        inc     dptr
        
        movx    a, @dptr    ; pos_x
        mov     r7, a       ;   Save unmodified pos_x
        anl     a, r5       ;   Apply mask; are we already in the sprite?
        jz      4$          ;   Yes. Skip the next test
        mov     a, r7       ;   Back to unmodified pos_x
        add     a, #(LCD_WIDTH - 1)
        jnc     13$         ;   Still nowhere on this line. Skip 3 bytes.
4$:     inc     dptr
          
        ; Activate sprite
        
        mov     a, r5       ; Save mask_x
        mov     @r0, a
        inc     r0

        mov     a, r7       ; Save pos_x
        mov     @r0, a
        inc     r0

        ; Calculate the index offset for the first tile on this line,
        ; accounting for any rows or columns that we are skipping.
        ; Since we support sprites up to 128x128 pixels, this just
        ; barely fits in 8 bits. This is stored in r1.
        
        mov     a, r6       ; Y offset
        swap    a           ; Pixels to tiles (>> 3)
        rl      a           
        anl     a, #0x1F

        mov	    b, r5	    ; Use the mask to calculate a tile width, rounded up
        jnb	    b.6, 24$    ;   <= 128 px
        jnb	    b.5, 23$    ;   <= 64px
        jnb	    b.4, 22$    ;   <= 32px
        jnb	    b.3, 21$    ;   <= 16px
        sjmp	20$         ;   <= 8px

24$:	rl	a
23$:	rl	a
22$:	rl	a
21$:	rl	a
20$:    mov     r1, a

        ; If the sprite overlaps the left side of the screen, we need to adjust
        ; r1 to account for the horizontal sprite position too.

        mov  	a, r7	    ; X test
        anl	a, r5
        jnz	7$
	
        mov     a, r7       ; X offset
        swap    a           ; Pixels to tiles (>> 3)
        rl      a           
        anl     a, #0x1F
        add	a, r1       ; Add Y contribution from above
        mov	r1, a
7$:

        ; Store lat2:lat1, as the original tile index plus our 8-bit adjustment.
        ; This is an annoying 8 + 7:7 addition. Similar to what we are doing in
        ; the radio decoder, but unsigned.
        
        movx    a, @dptr    ; Save lat1 (tile low)
        add     a, r1       ; Add (adj << 1)
        mov     b.0, c      ;   Save both carry flags
        add     a, r1
        mov     b.1, c
        mov     @r0, a
        inc     dptr
        inc     r0
        
        movx    a, @dptr    ; Save lat2 (tile high)
        jnb     b.0, 5$     ;   2x first carry flag
        add     a, #2
5$:     jnb     b.1, 6$     ;   2x second carry flag
        add     a, #2
6$:     mov     @r0, a
        inc     dptr
        inc     r0
        
        ; Calculate the Y contribution to our low address byte.
        ; This is the Y pixel offset * 32
        
        mov     a, r6
        swap    a
        rl      a
        anl     a, #0xE0
        mov     @r0, a
        inc     r0
        
        ; Is our sprite buffer full? Exit now.

        inc     _y_spr_active
        mov     a, _y_spr_active
        xrl     a, #_SYS_SPRITES_PER_LINE
        jz      3$
        
2$:     djnz    r2, 1$      ; Next iteration
        sjmp    3$
        
        ; Skipped sprite, with various numbers of bytes left in the struct
                
16$:    mov     a, _DPL     ; Skip 6 bytes (entire _SYSSpriteInfo)
        add     a, #6
        mov     _DPL, a
        sjmp    2$          ; Next iteration

14$:    mov     a, _DPL     ; Skip 4 bytes
        add     a, #4
        mov     _DPL, a
        sjmp    2$          ; Next iteration
        
13$:    mov     a, _DPL     ; Skip 3 bytes
        add     a, #3
        mov     _DPL, a
        sjmp    2$          ; Next iteration

3$:
    __endasm ;
}
