/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

// Start at a 2K segment boundary, so we can use ACALL in places.
#pragma codeseg BG1LINE

/*
 * Optimized scanline renderer for the BG0 + BG1 (configurable overlay) mode.
 *
 * This is implemented with a rather complicated unrolled state machine that
 * has a separate instance for each of the 8 possible inter-layer alignments.
 * It as much as possible processes pixels in wide bursts. Chroma-keying is
 * by far our slowest remaining operation, and we optimize that using an RLE
 * scheme that relies on End Of Line markers that Stir leaves for us.
 */

#include "graphics_bg1.h"
#include "hardware.h"

#define BG1_LOOP(lbl)                                           __endasm; \
    __asm djnz    R_LOOP_COUNT, lbl                             __endasm; \
    __asm ret                                                   __endasm; \
    __asm
    
// Next BG0 tile (while in BG0 state)
#define ASM_BG0_NEXT(lbl)                                       __endasm; \
    __asm inc   dptr                                            __endasm; \
    __asm inc   dptr                                            __endasm; \
    __asm ASM_X_WRAP_CHECK(lbl)                                 __endasm; \
    __asm ADDR_FROM_DPTR(_DPL)                                  __endasm; \
    __asm mov   ADDR_PORT, R_BG0_ADDR                           __endasm; \
    __asm

// Next BG0 tile (while in BG1 state)
// Can't use ASM_X_WRAP_CHECK here, since we need to redo CHROMA_PREP
// if we end up having to do a wrap, since the wrap_adjust routine
// will trash ACC.
#define ASM_BG0_NEXT_FROM_BG1(l2)                               __endasm; \
    __asm dec   _DPS                                            __endasm; \
    __asm inc   dptr                                            __endasm; \
    __asm inc   dptr                                            __endasm; \
    __asm inc   _DPS                                            __endasm; \
    __asm djnz  R_X_WRAP, l2                                    __endasm; \
    __asm lcall _vm_bg0_x_wrap_adjust                           __endasm; \
    __asm CHROMA_PREP()                                         __endasm; \
    __asm l2:                                                   __endasm; \
    __asm

/*
 * State transition to BG1 pixel
 */

#define DEF_STATE_BG1(x)                                        \
    __asm acall _state_bg1_func                                 __endasm; \
    __asm add   a, #((x) * 4)                                   __endasm; \
    __asm mov   ADDR_PORT, a                                    __endasm; \
    __asm CHROMA_PREP()                                         __endasm; \
    __asm ret                                                   __endasm;

static void state_bg1_func(void) __naked {
    __asm
        mov    _DPS, #1
        ADDR_FROM_DPTR(_DPL1)
        mov    a, R_BG1_ADDR
        ret
    __endasm ;
}       

void state_bg1_0(void) __naked {
    __asm
        mov    _DPS, #1
        ADDR_FROM_DPTR(_DPL1)
        mov    ADDR_PORT, R_BG1_ADDR
        CHROMA_PREP()
        ret
    __endasm ;
}

static void state_bg1_1(void) __naked { DEF_STATE_BG1(1) }
static void state_bg1_2(void) __naked { DEF_STATE_BG1(2) }
static void state_bg1_3(void) __naked { DEF_STATE_BG1(3) }
static void state_bg1_4(void) __naked { DEF_STATE_BG1(4) }
static void state_bg1_5(void) __naked { DEF_STATE_BG1(5) }
static void state_bg1_6(void) __naked { DEF_STATE_BG1(6) }
static void state_bg1_7(void) __naked { DEF_STATE_BG1(7) }

/*
 * State transition to BG0 pixel
 */
 
#define DEF_STATE_BG0(x)                                        \
    __asm acall _state_bg0_func                                 __endasm; \
    __asm add   a, #((x) * 4)                                   __endasm; \
    __asm mov   ADDR_PORT, a                                    __endasm; \
    __asm ret                                                   __endasm;
    
static void state_bg0_func(void) __naked {
    __asm
        mov    _DPS, #0
        ADDR_FROM_DPTR(_DPL)
        mov    a, R_BG0_ADDR
        ret
    __endasm ;
}    

void state_bg0_0(void) __naked {
    __asm
        mov    _DPS, #0
        ADDR_FROM_DPTR(_DPL)
        mov    ADDR_PORT, R_BG0_ADDR
        ret
    __endasm ;
}

static void state_bg0_1(void) __naked { DEF_STATE_BG0(1) }
static void state_bg0_2(void) __naked { DEF_STATE_BG0(2) }
static void state_bg0_3(void) __naked { DEF_STATE_BG0(3) }
static void state_bg0_4(void) __naked { DEF_STATE_BG0(4) }
static void state_bg0_5(void) __naked { DEF_STATE_BG0(5) }
static void state_bg0_6(void) __naked { DEF_STATE_BG0(6) }
static void state_bg0_7(void) __naked { DEF_STATE_BG0(7) }

/*
 * Eek, this little macro does a lot of things.
 *
 *   1. Addresses a pixel from BG1
 *   2. Does a chroma-key test
 *   3. If opaque, moves to the next BG1 pixel
 *   4. If transparent, draws a pixel from BG0
 *   5. Tests the EOL bit. If set, we draw the rest of the tile from BG0.
 *
 * This is also pretty space-critical, since we have 64 instances
 * of this macro. One per X-pixel per X-alignment. So, a lot of the
 * lower-bandwidth work is handled by function calls. Unfortunately
 * a lot needs to be placed here, just because of the explosion of
 * different combinations we've created.
 *
 * IMPORTANT: "x0" is the current state for BG0, within this pixel.
 *            "x1" is the _next_ state for BG1, after this pixel is done.
 */

// Without EOL test (to save code size) 
#define CHROMA_BG1_BG0(l1,l2, x0,x1)                            __endasm; \
    __asm CHROMA_J_OPAQUE(l1)                                   __endasm; \
    __asm   lcall x0                                            __endasm; \
    __asm   lcall _addr_inc4                                    __endasm; \
    __asm   lcall x1                                            __endasm; \
    __asm   sjmp l2                                             __endasm; \
    __asm l1:                                                   __endasm; \
    __asm   ASM_ADDR_INC4()                                     __endasm; \
    __asm l2:                                                   __endasm; \
    __asm

// With EOL test
#define CHROMA_BG1_BG0_EOL(l1,l2,l3, x0,x1, eolC,eolJ)          __endasm; \
    __asm CHROMA_J_OPAQUE(l1)                                   __endasm; \
    __asm   orl   ADDR_PORT, #2                                 __endasm; \
    __asm   jnb   BUS_PORT.6, l3                                __endasm; \
    __asm   lcall x0                                            __endasm; \
    __asm   lcall eolC                                          __endasm; \
    __asm   ljmp  eolJ                                          __endasm; \
    __asm l3:                                                   __endasm; \
    __asm   lcall x0                                            __endasm; \
    __asm   lcall _addr_inc4                                    __endasm; \
    __asm   lcall x1                                            __endasm; \
    __asm   sjmp l2                                             __endasm; \
    __asm l1:                                                   __endasm; \
    __asm   ASM_ADDR_INC4()                                     __endasm; \
    __asm l2:                                                   __endasm; \
    __asm

// ACALL, no EOL test
#define CHROMA_BG1_BG0_A(l1,l2, x0,x1)                          __endasm; \
    __asm CHROMA_J_OPAQUE(l1)                                   __endasm; \
    __asm   acall x0                                            __endasm; \
    __asm   acall _addr_inc4                                    __endasm; \
    __asm   acall x1                                            __endasm; \
    __asm   sjmp l2                                             __endasm; \
    __asm l1:                                                   __endasm; \
    __asm   ASM_ADDR_INC4()                                     __endasm; \
    __asm l2:                                                   __endasm; \
    __asm

// ACALL, with EOL test
#define CHROMA_BG1_BG0_EOL_A(l1,l2,l3, x0,x1, eolC,eolJ)        __endasm; \
    __asm CHROMA_J_OPAQUE(l1)                                   __endasm; \
    __asm   orl   ADDR_PORT, #2                                 __endasm; \
    __asm   jnb   BUS_PORT.6, l3                                __endasm; \
    __asm   acall x0                                            __endasm; \
    __asm   acall eolC                                          __endasm; \
    __asm   ajmp  eolJ                                          __endasm; \
    __asm l3:                                                   __endasm; \
    __asm   acall x0                                            __endasm; \
    __asm   acall _addr_inc4                                    __endasm; \
    __asm   acall x1                                            __endasm; \
    __asm   sjmp l2                                             __endasm; \
    __asm l1:                                                   __endasm; \
    __asm   ASM_ADDR_INC4()                                     __endasm; \
    __asm l2:                                                   __endasm; \
    __asm

#define CHROMA_BG1_TARGET(lbl)                                  __endasm; \
    __asm mov   _DPS, #1                                        __endasm; \
    __asm sjmp  lbl                                             __endasm; \
    __asm


void addr_inc32() __naked {
    /*
     * Common jump table for address increments.
     * (Yes, this is a C function with entry points in the middle...)
     *
     * This really belongs more in lcd.c, but it's here so we can include
     * it in our ACALL bank.
     */

    __asm
            .globl _addr_inc28
            .globl _addr_inc24
            .globl _addr_inc20
            .globl _addr_inc16
            .globl _addr_inc12
            .globl _addr_inc8
            .globl _addr_inc4
            .globl _addr_inc3
            .globl _addr_inc2
            .globl _addr_inc1
    
             ASM_ADDR_INC4()
_addr_inc28: ASM_ADDR_INC4()
_addr_inc24: ASM_ADDR_INC4()
_addr_inc20: ASM_ADDR_INC4()
_addr_inc16: ASM_ADDR_INC4()
_addr_inc12: ASM_ADDR_INC4()
_addr_inc8:  ASM_ADDR_INC4()
_addr_inc4:  inc    ADDR_PORT
_addr_inc3:  inc    ADDR_PORT
_addr_inc2:  inc    ADDR_PORT
_addr_inc1:  inc    ADDR_PORT
             ret

    __endasm ;
}    

static void vm_bg0_bg1_tiles_fast_p0(void) __naked
{
    __asm

10$:    BG1_JB(1$)
        acall   _addr_inc32
11$:    BG1_NEXT_BIT() ASM_BG0_NEXT(9$) BG1_UPDATE_BIT() BG1_LOOP(10$)

1$:     acall   _state_bg1_0

        CHROMA_BG1_BG0_EOL_A(30$,40$,50$, _state_bg0_0, _state_bg1_1, _addr_inc32, 13$)
        CHROMA_BG1_BG0_EOL_A(31$,41$,51$, _state_bg0_1, _state_bg1_2, _addr_inc28, 13$)
        CHROMA_BG1_BG0_EOL_A(32$,42$,52$, _state_bg0_2, _state_bg1_3, _addr_inc24, 13$)
        CHROMA_BG1_BG0_EOL_A(33$,43$,53$, _state_bg0_3, _state_bg1_4, _addr_inc20, 13$)
        CHROMA_BG1_BG0_EOL_A(34$,44$,54$, _state_bg0_4, _state_bg1_5, _addr_inc16, 13$)
        CHROMA_BG1_BG0_EOL_A(35$,45$,55$, _state_bg0_5, _state_bg1_6, _addr_inc12, 13$)
        CHROMA_BG1_BG0_EOL_A(36$,46$,56$, _state_bg0_6, _state_bg1_7, _addr_inc8,  13$)
        CHROMA_BG1_BG0_EOL_A(37$,47$,57$, _state_bg0_7, _state_bg1_0, _addr_inc4,  13$)
        
12$:    ASM_BG1_NEXT()
        acall _state_bg0_0
        ajmp 11$

13$:    CHROMA_BG1_TARGET(12$)

    __endasm ;
}

static void vm_bg0_bg1_tiles_fast_p1(void) __naked
{
    __asm
    
        BG1_JB(14$)
    
10$:    BG1_NEXT_BIT()
        ASM_ADDR_INC4()
        BG1_UPDATE_BIT() BG1_JB_L(1$)
12$:    acall   _addr_inc28
        ASM_BG0_NEXT(9$) BG1_LOOP(10$)
        
14$:    acall   _state_bg1_7
        ajmp    13$
1$:     acall   _state_bg1_0
        
        CHROMA_BG1_BG0_EOL_A(30$,40$,50$, _state_bg0_1, _state_bg1_1, _addr_inc28, 16$)
        CHROMA_BG1_BG0_A    (31$,41$,     _state_bg0_2, _state_bg1_2)
        CHROMA_BG1_BG0_EOL_A(32$,42$,52$, _state_bg0_3, _state_bg1_3, _addr_inc20, 16$)
        CHROMA_BG1_BG0_A    (33$,43$,     _state_bg0_4, _state_bg1_4)
        CHROMA_BG1_BG0_EOL_A(34$,44$,54$, _state_bg0_5, _state_bg1_5, _addr_inc12, 16$)
        CHROMA_BG1_BG0_A    (35$,45$,     _state_bg0_6, _state_bg1_6)
        CHROMA_BG1_BG0_A    (36$,46$,     _state_bg0_7, _state_bg1_7)
        
17$:    ASM_BG0_NEXT_FROM_BG1(8$) BG1_LOOP(13$)
13$:
        CHROMA_BG1_BG0_A(37$,47$, _state_bg0_0, _state_bg1_0)
        
18$:    BG1_NEXT_BIT() ASM_BG1_NEXT() BG1_UPDATE_BIT() BG1_JB_L(1$)
        acall   _state_bg0_1
        ajmp    12$
16$:    acall   _state_bg1_7
        sjmp    17$

    __endasm ;
}

static void vm_bg0_bg1_tiles_fast_p2(void) __naked
{
    __asm
    
        BG1_JB(14$)
    
10$:    BG1_NEXT_BIT()
        acall   _addr_inc8
        BG1_UPDATE_BIT() BG1_JB_L(1$)
12$:    acall   _addr_inc24
        ASM_BG0_NEXT(9$) BG1_LOOP(10$)
        
14$:    acall   _state_bg1_6
        ajmp    13$
1$:     acall   _state_bg1_0
        
        CHROMA_BG1_BG0_EOL_A(30$,40$,50$, _state_bg0_2, _state_bg1_1, _addr_inc24, 16$)
        CHROMA_BG1_BG0_A    (31$,41$,     _state_bg0_3, _state_bg1_2)
        CHROMA_BG1_BG0_EOL_A(32$,42$,52$, _state_bg0_4, _state_bg1_3, _addr_inc16, 16$)
        CHROMA_BG1_BG0_A    (33$,43$,     _state_bg0_5, _state_bg1_4)
        CHROMA_BG1_BG0_EOL_A(34$,44$,54$, _state_bg0_6, _state_bg1_5, _addr_inc8,  16$)
        CHROMA_BG1_BG0_A    (35$,45$,     _state_bg0_7, _state_bg1_6)
        
17$:    ASM_BG0_NEXT_FROM_BG1(8$) BG1_LOOP(13$)
13$:
        CHROMA_BG1_BG0_EOL_A(36$,46$,56$, _state_bg0_0, _state_bg1_7, _addr_inc8,  6$)
        CHROMA_BG1_BG0_A    (37$,47$,     _state_bg0_1, _state_bg1_0)
        
18$:    BG1_NEXT_BIT() ASM_BG1_NEXT() BG1_UPDATE_BIT() BG1_JB_L(1$)
        acall   _state_bg0_2
        ajmp    12$
16$:    acall   _state_bg1_6
        ajmp    17$
6$:     acall   _state_bg1_0
        sjmp    18$

    __endasm ;
}

static void vm_bg0_bg1_tiles_fast_p3(void) __naked
{
    __asm
    
        BG1_JB(14$)
    
10$:    BG1_NEXT_BIT()
        acall   _addr_inc12
        BG1_UPDATE_BIT() BG1_JB_L(1$)
12$:    acall   _addr_inc20
        ASM_BG0_NEXT(9$) BG1_LOOP(10$)
        
14$:    acall   _state_bg1_5
        ajmp    13$
1$:     acall   _state_bg1_0
        
        CHROMA_BG1_BG0_EOL_A(30$,40$,50$, _state_bg0_3, _state_bg1_1, _addr_inc20, 16$)
        CHROMA_BG1_BG0_A    (31$,41$,     _state_bg0_4, _state_bg1_2)
        CHROMA_BG1_BG0_EOL_A(32$,42$,52$, _state_bg0_5, _state_bg1_3, _addr_inc12, 16$)
        CHROMA_BG1_BG0_A    (33$,43$,     _state_bg0_6, _state_bg1_4)
        CHROMA_BG1_BG0_A    (34$,44$,     _state_bg0_7, _state_bg1_5)
        
17$:    ASM_BG0_NEXT_FROM_BG1(8$) BG1_LOOP(13$)
13$:
        CHROMA_BG1_BG0_EOL_A(35$,45$,55$, _state_bg0_0, _state_bg1_6, _addr_inc12, 6$)
        CHROMA_BG1_BG0_A    (36$,46$,     _state_bg0_1, _state_bg1_7)
        CHROMA_BG1_BG0_A    (37$,47$,     _state_bg0_2, _state_bg1_0)
        
18$:    BG1_NEXT_BIT() ASM_BG1_NEXT() BG1_UPDATE_BIT() BG1_JB_L(1$)
        acall   _state_bg0_3
        ajmp    12$
16$:    acall   _state_bg1_5
        ajmp    17$
6$:     acall   _state_bg1_0
        sjmp    18$

    __endasm ;
}

static void vm_bg0_bg1_tiles_fast_p4(void) __naked
{
    __asm
    
        BG1_JB(14$)
    
10$:    BG1_NEXT_BIT()
        acall   _addr_inc16
        BG1_UPDATE_BIT() BG1_JB_L(1$)
12$:    acall   _addr_inc16
        ASM_BG0_NEXT(9$) BG1_LOOP(10$)
        
14$:    acall   _state_bg1_4
        ajmp    13$
1$:     acall   _state_bg1_0
        
        CHROMA_BG1_BG0_EOL_A(30$,40$,50$, _state_bg0_4, _state_bg1_1, _addr_inc16, 16$)
        CHROMA_BG1_BG0_A    (31$,41$,     _state_bg0_5, _state_bg1_2)
        CHROMA_BG1_BG0_EOL_A(32$,42$,52$, _state_bg0_6, _state_bg1_3, _addr_inc8,  16$)
        CHROMA_BG1_BG0_A    (33$,43$,     _state_bg0_7, _state_bg1_4)
        
17$:    ASM_BG0_NEXT_FROM_BG1(8$) BG1_LOOP(13$)
13$:
        CHROMA_BG1_BG0_EOL_A(34$,44$,54$, _state_bg0_0, _state_bg1_5, _addr_inc16, 6$)
        CHROMA_BG1_BG0_A    (35$,45$,     _state_bg0_1, _state_bg1_6)
        CHROMA_BG1_BG0_EOL_A(36$,46$,56$, _state_bg0_2, _state_bg1_7, _addr_inc8,  6$)
        CHROMA_BG1_BG0_A    (37$,47$,     _state_bg0_3, _state_bg1_0)
        
18$:    BG1_NEXT_BIT() ASM_BG1_NEXT() BG1_UPDATE_BIT() BG1_JB_L(1$)
        acall   _state_bg0_4
        ajmp    12$
16$:    acall   _state_bg1_4
        ajmp    17$
6$:     acall   _state_bg1_0
        sjmp    18$

    __endasm ;
}

static void vm_bg0_bg1_tiles_fast_p5(void) __naked
{
    __asm
    
        BG1_JB(14$)
    
10$:    BG1_NEXT_BIT()
        acall   _addr_inc20
        BG1_UPDATE_BIT() BG1_JB_L(1$)
12$:    acall   _addr_inc12
        ASM_BG0_NEXT(9$) BG1_LOOP(10$)
        
14$:    acall   _state_bg1_3
        ajmp    13$
1$:     acall   _state_bg1_0
        
        CHROMA_BG1_BG0_EOL_A(30$,40$,50$, _state_bg0_5, _state_bg1_1, _addr_inc12, 16$)
        CHROMA_BG1_BG0_A    (31$,41$,     _state_bg0_6, _state_bg1_2)
        CHROMA_BG1_BG0_A    (32$,42$,     _state_bg0_7, _state_bg1_3)
        
17$:    ASM_BG0_NEXT_FROM_BG1(8$) BG1_LOOP(13$)
13$:
        CHROMA_BG1_BG0_EOL_A(33$,43$,53$, _state_bg0_0, _state_bg1_4, _addr_inc20, 6$)
        CHROMA_BG1_BG0_A    (34$,44$,     _state_bg0_1, _state_bg1_5)
        CHROMA_BG1_BG0_EOL_A(35$,45$,55$, _state_bg0_2, _state_bg1_6, _addr_inc12, 6$)
        CHROMA_BG1_BG0_A    (36$,46$,     _state_bg0_3, _state_bg1_7)
        CHROMA_BG1_BG0_A    (37$,47$,     _state_bg0_4, _state_bg1_0)
        
18$:    BG1_NEXT_BIT() ASM_BG1_NEXT() BG1_UPDATE_BIT() BG1_JB_L(1$)
        acall   _state_bg0_5
        ajmp    12$
16$:    acall   _state_bg1_3
        ajmp    17$
6$:     acall   _state_bg1_0
        sjmp 18$

    __endasm ;
}

static void vm_bg0_bg1_tiles_fast_p6(void) __naked
{
    __asm
    
        BG1_JB(14$)
    
10$:    BG1_NEXT_BIT()
        lcall   _addr_inc24
        BG1_UPDATE_BIT() BG1_JB_L(1$)
12$:    lcall   _addr_inc8
        ASM_BG0_NEXT(9$) BG1_LOOP(10$)
        
14$:    lcall   _state_bg1_2
        ljmp 13$
1$:     lcall   _state_bg1_0
        
        CHROMA_BG1_BG0_EOL(30$,40$,50$, _state_bg0_6, _state_bg1_1, _addr_inc8,  16$)
        CHROMA_BG1_BG0    (31$,41$,     _state_bg0_7, _state_bg1_2)
        
17$:    ASM_BG0_NEXT_FROM_BG1(8$) BG1_LOOP(13$)
13$:
        CHROMA_BG1_BG0_EOL(32$,42$,52$, _state_bg0_0, _state_bg1_3, _addr_inc24, 6$)
        CHROMA_BG1_BG0    (33$,43$,     _state_bg0_1, _state_bg1_4)
        CHROMA_BG1_BG0_EOL(34$,44$,54$, _state_bg0_2, _state_bg1_5, _addr_inc16, 6$)
        CHROMA_BG1_BG0    (35$,45$,     _state_bg0_3, _state_bg1_6)
        CHROMA_BG1_BG0_EOL(36$,46$,56$, _state_bg0_4, _state_bg1_7, _addr_inc8,  6$)
        CHROMA_BG1_BG0    (37$,47$,     _state_bg0_5, _state_bg1_0)
        
18$:    BG1_NEXT_BIT() ASM_BG1_NEXT() BG1_UPDATE_BIT() BG1_JB_L(1$)
        lcall   _state_bg0_6
        ljmp    12$
16$:    lcall   _state_bg1_2
        ljmp    17$
6$:     lcall   _state_bg1_0
        sjmp 18$

    __endasm ;
}

static void vm_bg0_bg1_tiles_fast_p7(void) __naked
{
    __asm
    
        BG1_JB(14$)
    
10$:    BG1_NEXT_BIT()
        lcall   _addr_inc28
        BG1_UPDATE_BIT() BG1_JB_L(1$)
12$:    ASM_ADDR_INC4()
        ASM_BG0_NEXT(9$) BG1_LOOP(10$)
        
14$:    lcall   _state_bg1_1
        ljmp    13$
1$:     lcall   _state_bg1_0
        
        CHROMA_BG1_BG0(30$,40$,  _state_bg0_7, _state_bg1_1)
        
17$:    ASM_BG0_NEXT_FROM_BG1(8$) BG1_LOOP(13$)
13$:
        CHROMA_BG1_BG0_EOL(31$,41$,51$, _state_bg0_0, _state_bg1_2, _addr_inc28, 6$)
        CHROMA_BG1_BG0    (32$,42$,     _state_bg0_1, _state_bg1_3)
        CHROMA_BG1_BG0_EOL(33$,43$,53$, _state_bg0_2, _state_bg1_4, _addr_inc20, 6$)
        CHROMA_BG1_BG0    (34$,44$,     _state_bg0_3, _state_bg1_5)
        CHROMA_BG1_BG0_EOL(35$,45$,55$, _state_bg0_4, _state_bg1_6, _addr_inc12, 6$)
        CHROMA_BG1_BG0    (36$,46$,     _state_bg0_5, _state_bg1_7)
        CHROMA_BG1_BG0    (37$,47$,     _state_bg0_6, _state_bg1_0)
        
18$:    BG1_NEXT_BIT() ASM_BG1_NEXT() BG1_UPDATE_BIT() BG1_JB_L(1$)
        lcall   _state_bg0_7
        ljmp    12$
6$:     lcall   _state_bg1_0
        sjmp 18$

    __endasm ;
}

void vm_bg0_bg1_tiles_fast(void)
{
    /*
     * Render R_LOOP_COUNT tiles from bg0, quickly, with bg1 overlaid
     * at the proper panning offset from x_bg1_offset.
     *
     * Instead of using a jump table here, we use a small binary tree.
     * There's no way to do a jump table without trashing DPTR, and this
     * is cheaper than saving/restoring DPTR.
     */

    /*
     * Start out in BG0 state, at the beginning of the tile.
     *
     * Use only the line portion of the Y address.
     * The pixel (column) portion is baked into these unrolled loops.
     */
     
    __asm
        lcall   _state_bg0_0
        mov     R_BG1_ADDR, _y_bg1_addr_l
    __endasm ;
    
    if (x_bg1_offset_bit2) {
        if (x_bg1_offset_bit1) {
            if (x_bg1_offset_bit0)
                vm_bg0_bg1_tiles_fast_p7();
            else
                vm_bg0_bg1_tiles_fast_p6();
        } else {
            if (x_bg1_offset_bit0)
                vm_bg0_bg1_tiles_fast_p5();
            else
                vm_bg0_bg1_tiles_fast_p4();
        }
    } else {
        if (x_bg1_offset_bit1) {
            if (x_bg1_offset_bit0)
                vm_bg0_bg1_tiles_fast_p3();
            else
                vm_bg0_bg1_tiles_fast_p2();
        } else {
            if (x_bg1_offset_bit0)
                vm_bg0_bg1_tiles_fast_p1();
            else
                vm_bg0_bg1_tiles_fast_p0();
        }
    }
}
 
void vm_bg0_bg1_pixels(void) __naked
{   
    /*
     * Basic pixel-by-pixel renderer for BG0 + BG1. This isn't particularly fast.
     * It's capable of rendering whole scanlines, but we only use it for partial tiles.
     *
     * Must be set up on entry:
     *   R_X_WRAP, R_BG0_ADDR, R_BG1_ADDR, R_LOOP_COUNT, DPTR, DPTR1, MDU, B
     */

    __asm
1$:
        BG1_JNB(2$)                 ; Chroma-key BG1 layer
        lcall   _state_bg1_0
        CHROMA_J_OPAQUE(3$)
2$:     lcall   _state_bg0_0        ; Draw BG0 layer
3$:     ASM_ADDR_INC4()

        BG0_NEXT_PIXEL(4$, 5$)
        BG1_NEXT_PIXEL(6$, 7$)

        djnz    R_LOOP_COUNT, 1$
        ret
    __endasm ;
}

void vm_bg0_bg1_line(void)
{
    /*
     * Top-level scanline renderer for BG0+BG1.
     * Segment the line into a fast full-tile burst, and up to two slower partial tiles.
     */
     
    __asm
        ; Load registers
     
        mov     dpl, _y_bg0_map
        mov     dph, (_y_bg0_map+1)
            
        mov     a, _y_bg0_addr_l
        add     a, _x_bg0_first_addr
        mov     R_BG0_ADDR, a
        
        mov     a, _y_bg1_addr_l
        add     a, _x_bg1_first_addr
        mov     R_BG1_ADDR, a

        mov     R_X_WRAP, _x_bg0_wrap

        BG1_UPDATE_BIT()
 
#ifdef BG1_SLOW_AND_STEADY
        ; Debugging only: Use the pixel renderer for everything
        mov     R_LOOP_COUNT, #LCD_WIDTH
        lcall   _vm_bg0_bg1_pixels
#else
        
        ; Render a partial tile at the beginning of the line, if we have one
        
        mov     a, _x_bg0_first_w
        jb      acc.3, 1$
        mov     R_LOOP_COUNT, a
        lcall   _vm_bg0_bg1_pixels
        
        ; We did render a partial tile; there are always 15 full tiles, then another partial.
        
        mov     R_LOOP_COUNT, #15
        lcall   _vm_bg0_bg1_tiles_fast
        mov     _DPS, #0

        mov     a, _y_bg1_addr_l
        add     a, _x_bg1_last_addr
        mov     R_BG1_ADDR, a
        mov     R_LOOP_COUNT, _x_bg0_last_w
        lcall   _vm_bg0_bg1_pixels
        sjmp    2$
        
        ; No partial tile? We are doing a fully aligned burst of 16 tiles.
1$:
        mov     R_LOOP_COUNT, #16
        lcall    _vm_bg0_bg1_tiles_fast       

#endif  // !BG1_SLOW_AND_STEADY
     
        ; Cleanup. Our renderers might not switch back to DPTR.
2$:       
        mov     _DPS, #0
        
    __endasm ;
}
