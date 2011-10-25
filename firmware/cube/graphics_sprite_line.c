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
    __asm    mov   a, XSPR_LAT1(id)                                 __endasm; \
    __asm    inc   a                                                __endasm; \
    __asm    inc   a                                                __endasm; \
    __asm    mov   XSPR_LAT1(id), a                                 __endasm; \
    __asm    jnz   2$                                               __endasm; \
    __asm      inc  XSPR_LAT2(id)                                   __endasm; \
    __asm      inc  XSPR_LAT2(id)                                   __endasm; \
    __asm    2$:                                                    __endasm; \
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
     * Basic pixel-by-pixel renderer for BG0 + SPR. Not especially
     * fast, but it's thorough.
     *
     * Must be set up on entry:
     *   R_SPR_ACTIVE, R_X_WRAP, R_BG0_ADDR, R_LOOP_COUNT, DPTR
     */

    __asm
        
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
        SPR_BG_BURST_TEST(0, 5$) SPR_SKIP(1, 30$, 36$)
        SPR_BG_BURST_TEST(1, 5$) SPR_SKIP(2, 31$, 36$)
        SPR_BG_BURST_TEST(2, 5$) SPR_SKIP(3, 32$, 36$)
        SPR_BG_BURST_TEST(3, 5$)
36$:
        
        ; Make sure we have enough pixel loops remaining
        mov     a, R_LOOP_COUNT
        add     a, #(0x100 - 8)
        jc      6$

5$:        
        djnz    R_LOOP_COUNT, 3$
37$:    ret
3$:     ljmp    1$
        
        ; --- BG0 Tile Burst

6$:        
        mov     R_LOOP_COUNT, a
      
        ADDR_FROM_DPTR(_DPL)
        mov     ADDR_PORT, R_BG0_ADDR
        lcall    _addr_inc32

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

void vm_bg0_spr_bg1_pixels() __naked
{
    /*
     * Basic pixel-by-pixel renderer for BG0 + SPR + BG1. Not especially
     * fast, but it's thorough.
     *
     * Must be set up on entry:
     *   R_SPR_ACTIVE, R_X_WRAP, R_BG0_ADDR, R_BG1_ADDR, R_LOOP_COUNT, DPTR, DPTR1, MDU, B
     */

    __asm
        
1$:
        ; Overlay BG1        
        BG1_JNB(5$)
        lcall   _state_bg1_0
        CHROMA_J_OPAQUE(40$)
5$:

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
        
        ; Update BG state
        BG0_NEXT_PIXEL(50$, 51$)
        BG1_NEXT_PIXEL(52$, 53$)
        
        djnz    R_LOOP_COUNT, 3$
        mov     _DPS, #0
        ret
3$:     ljmp    1$
        
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
        mov     R_SPR_ACTIVE, _y_spr_active
        mov     R_LOOP_COUNT, #128        
    __endasm ;
    
    vm_bg0_spr_pixels();
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
