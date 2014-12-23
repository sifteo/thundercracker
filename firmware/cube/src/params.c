/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
        mov     a, #HWREV
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
        movx    @dptr, a
        clr     _FSR_WEN

        sjmp    4$                  ; Test byte, make sure it is not 0xFF

2$:     inc     dptr
        mov     a, _DPL
        cjne    a, #(PARAMS_HWID + HWID_LEN), 4$

        mov     _RNGCTL, #0         ; RNG back to sleep

    __endasm ;
}
