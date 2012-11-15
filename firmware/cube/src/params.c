/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "cube_hardware.h"
#include "params.h"


void params_init()
{
    /*
     * Generate a hardware ID, if necessary.
     *
     * More accurately, any HWID bytes that are not yet programmed
     * get programmed with a true random value. So, if there's a power fail
     * during this operation, we'll continue where we left off.
     *
     * MUST be called before interrupts are enabled. We don't want
     * to enter ISRs while FSR_WEN is set!
     *
     * Inside the HWID, we embed the cube version as the first byte.
     */

    __asm

        mov     dptr, #PARAMS_HWID

        ;--------------------------------------------------------------------
        ; Revision Byte
        ;--------------------------------------------------------------------

        movx    a, @dptr
        inc     a
        jnz     1$                  ; Skip if already programmed

        setb    _FSR_WEN
        mov     a, #CUBE_VERSION_LATEST
        movx    @dptr, a
        clr     _FSR_WEN

1$:
        inc     dptr

        ;--------------------------------------------------------------------
        ; Random bytes
        ;--------------------------------------------------------------------

        mov     _RNGCTL, #0xC0      ; Power on RNG, with bias corrector

4$:     movx    a, @dptr
        inc     a
        jnz     2$                  ; Skip if already programmed

3$:     mov     a, _RNGCTL          ; Wait for RNG
        jnb     acc.5, 3$

        setb    _FSR_WEN            ; Program value from RNG
        mov     a, _RNGDAT
        jz      3$                  ; 0xff is  not permissible
        cpl     a
        movx    @dptr, a
        clr     _FSR_WEN

2$:     inc     dptr
        mov     a, _DPL
        cjne    a, #(PARAMS_HWID + HWID_LEN), 4$

        mov     _RNGCTL, #0         ; RNG back to sleep

    __endasm ;
}
