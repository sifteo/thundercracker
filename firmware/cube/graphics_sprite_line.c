/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "graphics_sprite.h"

   
// Sprite mask test, with position update. Branches if inside the sprite.
#define SPR_TEST(id, lbl)                                           __endasm; \
    __asm   mov     a, XSPR_MASK(id)                                __endasm; \
    __asm   anl     a, XSPR_POS(id)                                 __endasm; \
    __asm   inc     XSPR_POS(id)                                    __endasm; \
    __asm   jz      lbl                                             __endasm; \
    __asm
    
// Advance a sprite's position by an arbitrary amount
#define SPR_ADVANCE(id, amount)                                     __endasm; \
    __asm   mov     a, XSPR_POS(id)                                 __endasm; \
    __asm   add     a, #(amount)                                    __endasm; \
    __asm   mov     XSPR_POS(id), a                                 __endasm; \
    __asm
    
// Can we safely perform a background burst behind this sprite?
// The sprite must be inactive now and for the next 8 pixels.
#define SPR_BG_BURST_TEST(id, lblNotOk)                             __endasm; \
    __asm   mov     R_SCRATCH, XSPR_POS(id)                         __endasm; \
    __asm   mov     a, XSPR_MASK(id)                                __endasm; \
    __asm   anl     a, R_SCRATCH                                    __endasm; \
    __asm   jz      lblNotOk                                        __endasm; \
    __asm   mov     a, #8                                           __endasm; \
    __asm   add     a, R_SCRATCH                                    __endasm; \
    __asm   jc      lblNotOk                                        __endasm; \
    __asm
    
// If we're done with the active sprites, jump ahead
#define SPR_SKIP(id, l1, end)                                       __endasm; \
    __asm   cjne    R_SPR_ACTIVE, #(id), l1                         __endasm; \
    __asm   ljmp    end                                             __endasm; \
    __asm   l1:                                                     __endasm; \
    __asm

// Load lat1/lat2 from x_spr[id]
#define SPR_ADDRESS_LAT(id)                                         __endasm; \
    __asm   mov     ADDR_PORT, XSPR_LAT1(id)                        __endasm; \
    __asm   mov     CTRL_PORT, #(CTRL_FLASH_OUT | CTRL_FLASH_LAT1)  __endasm; \
    __asm   mov     ADDR_PORT, XSPR_LAT2(id)                        __endasm; \
    __asm   mov     CTRL_PORT, #(CTRL_FLASH_OUT | CTRL_FLASH_LAT2)  __endasm; \
    __asm
    
// Increment a sprite's tile address (lat1/lat2)
#define SPR_ADDRESS_LAT_INC(id, l1)                                 __endasm; \
    __asm    mov   a, XSPR_LAT1(id)                                 __endasm; \
    __asm    inc   a                                                __endasm; \
    __asm    inc   a                                                __endasm; \
    __asm    mov   XSPR_LAT1(id), a                                 __endasm; \
    __asm    jnz   l1                                               __endasm; \
    __asm      inc  XSPR_LAT2(id)                                   __endasm; \
    __asm      inc  XSPR_LAT2(id)                                   __endasm; \
    __asm    l1:                                                    __endasm; \
    __asm
    
// Load a full sprite address, and update lat1/lat2
#define SPR_ADDRESS_NEXT(id)                                        \
    __asm   SPR_ADDRESS_LAT(id)                                     __endasm; \
    __asm   mov    a, XSPR_POS(id)                                  __endasm; \
    __asm   dec    a                                                __endasm; \
    __asm   rl     a                                                __endasm; \
    __asm   rl     a                                                __endasm; \
    __asm   anl    a, #0x1C                                         __endasm; \
    __asm   cjne   a, #0x1C, 1$                                     __endasm; \
    __asm    add   a, XSPR_LINE_ADDR(id)                            __endasm; \
    __asm    mov   ADDR_PORT, a                                     __endasm; \
    __asm    SPR_ADDRESS_LAT_INC(id, 2$)                            __endasm; \
    __asm    CHROMA_PREP()                                          __endasm; \
    __asm    ret                                                    __endasm; \
    __asm   1$:                                                     __endasm; \
    __asm   add   a, XSPR_LINE_ADDR(id)                             __endasm; \
    __asm   mov   ADDR_PORT, a                                      __endasm; \
    __asm   CHROMA_PREP()                                           __endasm; \
    __asm   ret                                                     __endasm; \
    
static void spr_address_next_0() __naked { SPR_ADDRESS_NEXT(0) }
static void spr_address_next_1() __naked { SPR_ADDRESS_NEXT(1) }
static void spr_address_next_2() __naked { SPR_ADDRESS_NEXT(2) }
static void spr_address_next_3() __naked { SPR_ADDRESS_NEXT(3) }

// Perform state updates on a sprite that we skipped. Updates its position,
// and if applicable updates lat1/lat2.
#define SPR_OCCLUDED_NEXT(id)                                       \
    __asm   SPR_TEST(id, 1$)                                        __endasm; \
    __asm   2$: ret                                                 __endasm; \
    __asm   1$:                                                     __endasm; \
    __asm   mov   a, XSPR_POS(id)                                   __endasm; \
    __asm   anl   a, #7                                             __endasm; \
    __asm   jnz   2$                                                __endasm; \
    __asm   mov   a, XSPR_LAT1(id)                                  __endasm; \
    __asm   inc   a                                                 __endasm; \
    __asm   inc   a                                                 __endasm; \
    __asm   mov   XSPR_LAT1(id), a                                  __endasm; \
    __asm   jnz   2$                                                __endasm; \
    __asm   inc  XSPR_LAT2(id)                                      __endasm; \
    __asm   inc  XSPR_LAT2(id)                                      __endasm; \
    __asm   ret                                                     __endasm; \

static void spr_occluded_next_0() __naked { SPR_OCCLUDED_NEXT(0) }
static void spr_occluded_next_1() __naked { SPR_OCCLUDED_NEXT(1) }
static void spr_occluded_next_2() __naked { SPR_OCCLUDED_NEXT(2) }
static void spr_occluded_next_3() __naked { SPR_OCCLUDED_NEXT(3) }


void vm_bg0_spr_pixels() __naked
{
    /*
     * Pixel-by-pixel renderer for BG0 + SPR. Supports any combination
     * of sprites and BG0. Detects groups of 8 BG0 pixels, and forms bursts
     *
     * Must be set up on entry:
     *   R_SPR_ACTIVE, R_X_WRAP, R_BG0_ADDR, R_LOOP_COUNT, DPTR
     */

    __asm
    
        ; Can we start immediately into a burst?
        
        mov     a, R_BG0_ADDR
        anl     a, #0x1F
        jnz     1$               ; No
        ljmp    38$              ; Yes
        
1$:     ; Test all active sprites
        
        SPR_TEST(0, 10$) 14$: SPR_SKIP(1, 20$, 2$)
        SPR_TEST(1, 11$) 15$: SPR_SKIP(2, 21$, 2$)
        SPR_TEST(2, 12$) 16$: SPR_SKIP(3, 22$, 2$)
        SPR_TEST(3, 13$) sjmp 2$

10$:    lcall   _spr_address_next_0  CHROMA_LONG(23$, 40$, 14$)
11$:    lcall   _spr_address_next_1  CHROMA_LONG(24$, 41$, 15$)
12$:    lcall   _spr_address_next_2  CHROMA_LONG(25$, 42$, 16$)
13$:    lcall   _spr_address_next_3  CHROMA_LONG(26$, 4$, 2$)

40$:    SPR_SKIP(1, 27$, 4$)  lcall   _spr_occluded_next_1
41$:    SPR_SKIP(2, 28$, 4$)  lcall   _spr_occluded_next_2
42$:    SPR_SKIP(3, 29$, 4$)  lcall   _spr_occluded_next_3
        sjmp 4$

2$:     ; Background BG0        
        ADDR_FROM_DPTR(_DPL)
        mov     ADDR_PORT, R_BG0_ADDR
4$:     ASM_ADDR_INC4()
        dec     R_LOOP_COUNT
        
        ; Update BG0 state, and look for opportunities to burst
        ; a whole BG0 tile at a time. This is based on BG0_NEXT_PIXEL.
        
        mov     a, R_BG0_ADDR           ; Quick addition, move to the next pixel
        add     a, #4
        mov     R_BG0_ADDR, a
        anl     a, #0x1F
        jnz     5$
        
        mov     a, R_BG0_ADDR           ; Overflow compensation
        add     a, #(0x100 - 0x20)
        mov     R_BG0_ADDR, a
        
7$:     inc     dptr                    ; Next BG0 tile
        inc     dptr
        ASM_X_WRAP_CHECK(8$)

        ; Make sure all sprites are out of the way of this potential burst
38$:    SPR_BG_BURST_TEST(0, 5$) SPR_SKIP(1, 30$, 36$)
        SPR_BG_BURST_TEST(1, 5$) SPR_SKIP(2, 31$, 36$)
        SPR_BG_BURST_TEST(2, 5$) SPR_SKIP(3, 32$, 36$)
        SPR_BG_BURST_TEST(3, 5$)
36$:
        
        ; Make sure we have enough pixel loops remaining
        mov     a, R_LOOP_COUNT
        add     a, #(0x100 - 8)
        jc      6$

5$:        
        cjne    R_LOOP_COUNT, #0, 3$
37$:    ret
3$:     ljmp    1$
        
        ; --- BG0 Tile Burst

6$:        
        mov     R_LOOP_COUNT, a
        ADDR_FROM_DPTR(_DPL)
        mov     ADDR_PORT, R_BG0_ADDR
        lcall   _addr_inc32

        SPR_ADVANCE(0, 8) SPR_SKIP(1, 33$, 9$)
        SPR_ADVANCE(1, 8) SPR_SKIP(2, 34$, 9$)
        SPR_ADVANCE(2, 8) SPR_SKIP(3, 35$, 9$)
        SPR_ADVANCE(3, 8)
9$:
        
        mov     a, R_LOOP_COUNT
        jz      37$
        ljmp    7$
        
    __endasm ;
}

void vm_bg0_spr1_fast() __naked
{
    /*
     * Speedy single-purpose renderer; BG0 plus exactly one visible sprite.
     *
     * Must be set up on entry:
     *   R_X_WRAP, R_BG0_ADDR, DPTR
     */

#define  R_LEFT       r0
#define  R_SPR_ADDR   r4
#define  R_RIGHT      r7
     
    __asm

        ; -------- Setup
    
        ; Do we have any BG0 pixels to the left of our sprite?
        ; Do a sprite mask test. If we are not inside the sprite
        ; already, we can calculate how far away it is by negating
        ; the position.
        
        mov     R_RIGHT, XSPR_POS(0)
        mov     a, XSPR_MASK(0)
        anl     a, R_RIGHT
        jnz     1$

        ; Already in the sprite, adjust R_LEFT before jumping
        ; into the middle of the sprite code.
        
        mov     a, XSPR_MASK(0)
        add     a, R_RIGHT
        mov     R_LEFT, a

        mov     a, XSPR_POS(0)
        rl      a
        rl      a
        anl     a, #0x1C
        add     a, XSPR_LINE_ADDR(0)
        mov     R_SPR_ADDR, a
        
        mov     R_RIGHT, #0x80
        ljmp    12$
        
        ; Not already in the sprite. Calculate distance.
        
1$:     mov     a, R_RIGHT      
        cpl     a               ; Twos complement negation
        inc     a
        mov     R_LEFT, a
        
        mov     a, #LCD_WIDTH
        clr     c
        subb    a, R_LEFT
        mov     R_RIGHT, a
        
        ; -------- BG0, Left of the sprite

        ; Render some of BG0 to the left of our sprite. This is
        ; analogous to the partial tile that always exists at the
        ; left side of a normal BG0 scanline, where the partial tile
        ; is between 1 and 8 pixels wide. Its width here will be:
        ;
        ;   MIN(R_LEFT, x_bg0_first_w).
            
        ADDR_FROM_DPTR(_DPL);
        mov     ADDR_PORT, R_BG0_ADDR
        
        mov     a, R_LEFT
        clr     c
        subb    a, _x_bg0_first_w
        jnc      2$

        ; Borrow; this is the entirety of R_LEFT.
3$:     ASM_ADDR_INC4()             
        djnz    R_LEFT, 3$
        sjmp    6$

        ; No borrow; draw only x_bg0_first_w pixels.
2$:     mov     R_LEFT, a
        mov     a, _x_bg0_first_w   ; Draw first partial tile
5$:     ASM_ADDR_INC4()
        djnz    acc, 5$

        ; See if we can draw any full-tile bursts

        mov     R_BG0_ADDR, _y_bg0_addr_l
8$:     inc     dptr
        inc     dptr
        ASM_X_WRAP_CHECK(41$)
        ADDR_FROM_DPTR(_DPL);
        mov     ADDR_PORT, R_BG0_ADDR     
        mov     a, R_LEFT
        add     a, #(0x100 - 8)
        jnc     7$
        mov     R_LEFT, a
        lcall   _addr_inc32
        sjmp    8$
7$:
        
        ; Finish any partial tiles on the left side of the sprite
        
        mov     a, R_LEFT
        jz      6$
9$:     ASM_ADDR_INC4()
        djnz    R_LEFT, 9$
6$:     mov     R_BG0_ADDR, ADDR_PORT       ; Save the current X addr

        ; We are now positioned at the beginning of the sprite
        mov     XSPR_POS(0), #0

        ; -------- Sprite with chroma key

        mov     R_LEFT, XSPR_MASK(0)        ; R_LEFT tracks size of sprite
        mov     R_SPR_ADDR, XSPR_LINE_ADDR(0)
12$:    
        SPR_ADDRESS_LAT(0)                  ; Load lat1/lat2
        mov     ADDR_PORT, R_SPR_ADDR

        ; Sprite Pixel loop

14$:    
        mov     ADDR_PORT, R_SPR_ADDR
        CHROMA_PREP()
        CHROMA_J_OPAQUE(24$)

        ADDR_FROM_DPTR(_DPL)                ; Transparent path (BG0)
        mov     ADDR_PORT, R_BG0_ADDR
        ASM_ADDR_INC4()
        SPR_ADDRESS_LAT(0)
        mov     ADDR_PORT, R_SPR_ADDR

        djnz    R_RIGHT, 15$                ; Test for edge of screen
        ret        

24$:    ASM_ADDR_INC4()

        djnz    R_RIGHT, 15$                ; Test for edge of screen
        ret        
15$:    

        mov     a, R_BG0_ADDR               ; Update X portion of BG0 address
        add     a, #4
        mov     R_BG0_ADDR, a
        anl     a, #0x1F
        jnz     23$    
        mov     a, R_BG0_ADDR               ; Compensate for BG0 overflow
        add     a, #(0x100 - 0x20)
        mov     R_BG0_ADDR, a
        inc     dptr                        ; Next BG0 tile
        inc     dptr
        ASM_X_WRAP_CHECK(44$)
23$:

        mov     a, R_SPR_ADDR               ; Update X portion of sprite address
        add     a, #4
        mov     R_SPR_ADDR, a
        anl     a, #0x1F
        jnz     25$
        mov     a, R_SPR_ADDR               ; Compensate for X overflow
        add     a, #(0x100 - 0x20)
        mov     R_SPR_ADDR, a
        SPR_ADDRESS_LAT_INC(0, 45$)
        SPR_ADDRESS_LAT(0)
25$:

        mov     a, R_LEFT                   ; Test for edge of sprite
        inc     a
        mov     R_LEFT, a
        jnz     14$   
        
        ; -------- BG0, Right of the sprite
                
        ; See if we have a partial tile to draw, in order to get back
        ; into alignment with BG0
    
        mov     a, R_BG0_ADDR
        anl     a, #0x1F
        jz      16$

        ; If there is a partial tile, we need to update DPTR
        ASM_ADDR_FROM_DPTR_INC()
        ASM_X_WRAP_CHECK(43$)
        mov     ADDR_PORT, R_BG0_ADDR
        
        mov     a, R_BG0_ADDR
17$:    anl     a, #0x1F                    ; Look for X wrap
        jz      16$
        ASM_ADDR_INC4()
        djnz     R_RIGHT, 18$               ; Test for edge of screen
        ret        
18$:    add     a, #4
        sjmp    17$
16$:

        ; Now render as many tile bursts as possible

        mov     R_BG0_ADDR, _y_bg0_addr_l
19$:    ASM_ADDR_FROM_DPTR_INC();
        ASM_X_WRAP_CHECK(42$)
        mov     ADDR_PORT, R_BG0_ADDR     
        mov     a, R_RIGHT
        add     a, #(0x100 - 8)
        jnc     20$
        mov     R_RIGHT, a
        lcall   _addr_inc32
        sjmp    19$
20$:
        
        ; Finish any partial tiles on the right edge of the screen
        
        mov     a, R_RIGHT
        jz      21$
22$:    ASM_ADDR_INC4()
        djnz    R_RIGHT, 22$
21$:    ret

    __endasm ;
}

void vm_bg0_spr_bg1_pixels() __naked
{
    /*
     * Pixel-by-pixel renderer for BG0 + SPR + BG1. Supports any combination
     * of sprites, BG0, and BG1 tiles. Detects groups of 8 BG0+BG1 pixels, and forms bursts.
     * We use vm_bg0_bg1_tiles_fast() to do our BG1 rendering, so all bursts are synch'ed
     * to BG0 tile boundaries.
     *
     * Must be set up on entry:
     *   R_SPR_ACTIVE, R_X_WRAP, R_BG0_ADDR, R_BG1_ADDR, R_LOOP_COUNT, DPTR, DPTR1, MDU, B
     */

    __asm
        
        ; Can we start immediately into a burst?
        
        mov     a, R_BG0_ADDR
        anl     a, #0x1F
        jnz     1$               ; No
        ljmp    38$              ; Yes
        
1$:
        ; Overlay BG1        
        BG1_JNB(44$)
        lcall   _state_bg1_0
        CHROMA_J_OPAQUE(40$)
44$:

        ; Test all active sprites   
        SPR_TEST(0, 10$) 14$: SPR_SKIP(1, 20$, 2$)
        SPR_TEST(1, 11$) 15$: SPR_SKIP(2, 21$, 2$)
        SPR_TEST(2, 12$) 16$: SPR_SKIP(3, 22$, 2$)
        SPR_TEST(3, 13$) ljmp 2$

10$:    lcall   _spr_address_next_0  CHROMA_LONG(23$, 41$, 14$)
11$:    lcall   _spr_address_next_1  CHROMA_LONG(24$, 42$, 15$)
12$:    lcall   _spr_address_next_2  CHROMA_LONG(25$, 43$, 16$)
13$:    lcall   _spr_address_next_3  CHROMA_LONG(26$, 4$, 2$)

40$:    lcall   _spr_occluded_next_0
41$:    SPR_SKIP(1, 27$, 4$)  lcall   _spr_occluded_next_1
42$:    SPR_SKIP(2, 28$, 4$)  lcall   _spr_occluded_next_2
43$:    SPR_SKIP(3, 29$, 4$)  lcall   _spr_occluded_next_3
        sjmp 4$

2$:     ; Background BG0        
        lcall   _state_bg0_0
4$:     ASM_ADDR_INC4()
        dec     R_LOOP_COUNT
        
        ; Update BG1 state in the usual way
        BG1_NEXT_PIXEL(52$, 53$)

        ; Update BG0 state, and look for opportunities to burst
        ; a whole BG0 tile at a time. This is based on BG0_NEXT_PIXEL.
        
        mov     a, R_BG0_ADDR           ; Quick addition, move to the next pixel
        add     a, #4
        mov     R_BG0_ADDR, a
        anl     a, #0x1F
        jnz     5$
        
        mov     a, R_BG0_ADDR           ; Overflow compensation
        add     a, #(0x100 - 0x20)
        mov     R_BG0_ADDR, a
        
7$:     mov     _DPS, #0
        inc     dptr                    ; Next BG0 tile
        inc     dptr
        ASM_X_WRAP_CHECK(8$)

        ; Make sure all sprites are out of the way of this potential burst
38$:    SPR_BG_BURST_TEST(0, 5$) SPR_SKIP(1, 30$, 36$)
        SPR_BG_BURST_TEST(1, 5$) SPR_SKIP(2, 31$, 36$)
        SPR_BG_BURST_TEST(2, 5$) SPR_SKIP(3, 32$, 36$)
        SPR_BG_BURST_TEST(3, 5$)
36$:
        
        ; Make sure we have enough pixel loops remaining
        mov     a, R_LOOP_COUNT
        add     a, #(0x100 - 8)
        jc      6$

5$:        
        cjne    R_LOOP_COUNT, #0, 3$
37$:    mov     _DPS, #0
        ret
3$:     ljmp    1$
        
        ; --- BG0 Tile Burst

6$:        
        mov     R_SCRATCH, a        ; Save our new loop count
        mov     R_LOOP_COUNT, #1    ; This is measured in tiles
        
        lcall   _vm_bg0_bg1_tiles_fast
        
        mov     a, R_SCRATCH        ; Restore loop count
        mov     R_LOOP_COUNT, a
        
        mov     a, _y_bg1_addr_l    ; Calculate correct new BG1 tile address
        add     a, _x_bg1_last_addr
        mov     R_BG1_ADDR, a

        SPR_ADVANCE(0, 8) SPR_SKIP(1, 33$, 9$)
        SPR_ADVANCE(1, 8) SPR_SKIP(2, 34$, 9$)
        SPR_ADVANCE(2, 8) SPR_SKIP(3, 35$, 9$)
        SPR_ADVANCE(3, 8)
9$:
        
        mov     a, R_LOOP_COUNT
        jz      37$
        ljmp    38$        
        
    __endasm ;
}

void vm_bg0_spr_line()
{
    __asm
        mov     dpl, _y_bg0_map
        mov     dph, (_y_bg0_map+1)

        mov     a, _x_bg0_first_addr
        add     a, _y_bg0_addr_l
        mov     R_BG0_ADDR, a
        
        mov     R_X_WRAP, _x_bg0_wrap
    __endasm ;
    
    if (y_spr_active > 1) {
        /*
         * Two or more sprites. Do a slower pixel-by-pixel render,
         * so we can get all of the multi-layered chroma keying right.
         */
         
        __asm
            mov     R_SPR_ACTIVE, _y_spr_active
            mov     R_LOOP_COUNT, #128        
        __endasm ;
        
        vm_bg0_spr_pixels();

    } else {
        /*
         * Just BG0 and one sprite. Take the fast path.
         */
         
        vm_bg0_spr1_fast();
    }
}

void vm_bg0_spr_bg1_line()
{
    __asm
        mov     dpl, _y_bg0_map
        mov     dph, (_y_bg0_map+1)
            
        mov     a, _y_bg0_addr_l
        add     a, _x_bg0_first_addr
        mov     R_BG0_ADDR, a
        
        mov     a, _y_bg1_addr_l
        add     a, _x_bg1_first_addr
        mov     R_BG1_ADDR, a

        BG1_UPDATE_BIT()

        mov     R_X_WRAP, _x_bg0_wrap
        mov     R_SPR_ACTIVE, _y_spr_active
        mov     R_LOOP_COUNT, #128
    __endasm ;
    
    vm_bg0_spr_bg1_pixels();
}
