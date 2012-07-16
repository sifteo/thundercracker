/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "lcd.h"
#include "draw.h"

void draw_clear()
{
    // Clear all of VRAM (1 kB)

    __asm
        mov     dptr, #0
1$:     clr     a

        movx    @dptr, a
        inc     dptr

        mov     a, #(1024 >> 8)
        cjne    a, dph, 1$
    __endasm ;

    vram.num_lines = 128;
    vram.mode = _SYS_VM_BG0_ROM;
    vram.flags = _SYS_VF_TOGGLE;

    // Reset state
    draw_xy = XY(0, 0);
}

void draw_image(const __code uint8_t *image)
{
    image = image;
    __asm
        mov     _DPL1, _draw_xy
        mov     _DPH1, (_draw_xy + 1)

        ; Read header

        clr     a               ; Width
        movc    a, @a+DPTR
        mov     r1, a
        inc     dptr

        clr     a               ; Height
        movc    a, @a+DPTR
        mov     r2, a
        inc     dptr

        clr     a               ; MSB / Mode
        movc    a, @a+DPTR
        mov     r3, a
        inc     dptr

        
1$:                             ; Loop over all lines
        mov     ar0, r1         ; Loop over tiles in this line
2$:

        clr     a               ; Read LSB
        movc    a, @a+DPTR
        inc     dptr
        clr     c               ; Tile index to low 7 bits, with extra bit in C
        rlc     a

        mov     _DPS, #1        ; Write low byte to destination
        movx    @dptr, a
        inc     dptr

        mov     a, r3           ; Test mode: Is the source 8-bit or 16-bit?
        inc     a
        jz      3$              ; Jump if 16-bit

        ; 8-bit source

        mov     a, r3           ; Start with common MSB
        rlc     a               ; Extra bit from C
        rl      a               ; Index to 7:7
        movx    @dptr, a        ; Write to high byte
        inc     dptr
        mov     _DPS, #0        ; Next source byte
        djnz    r0, 2$          ; Next tile
        sjmp    4$

        ; 16-bit source
3$:
        mov     _DPS, #0        ; Get another source byte
        clr     a
        movc    a, @a+DPTR
        inc     dptr
        mov     _DPS, #1

        rlc     a               ; Rotate in extra bit from C
        rl      a               ; Index to 7:7
        movx    @dptr, a        ; Write to high byte
        inc     dptr
        mov     _DPS, #0        ; Next source byte

        djnz    r0, 2$          ; Next tile
4$:

        ; Next line, adjust destination address by (PITCH - width)

        clr    c
        mov    a, #_SYS_VRAM_BG0_WIDTH
        subb   a, r1
        rl     a

        add    a, _DPL1
        mov    _DPL1, a
        mov    a, _DPH1
        addc   a, #0
        mov    _DPH1, a

        djnz    r2, 1$

        mov     _draw_xy, _DPL1
        mov     (_draw_xy + 1), _DPH1

    __endasm ;  
}
