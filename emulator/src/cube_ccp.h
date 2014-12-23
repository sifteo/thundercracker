/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Micah Elizabeth Scott <micah@misc.name>
 *
 * Copyright <c> 2012 Sifteo, Inc.
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

/*
 * Implements the nRF24LE1's Crypto Co-Processor unit (CCP), a Galois Field
 * multiplier that's intended to accelerate software implementations of AES.
 *
 * (Naturally, our firmware uses this peripheral for other purposes...)
 */
 
#ifndef _CUBE_CCP_H
#define _CUBE_CCP_H

#include <stdint.h>
#include "cube_cpu.h"

namespace Cube {


class CCP {
public:

    ALWAYS_INLINE int read(CPU::em8051 &cpu)
    {
        return galoisFieldMultiply(cpu.mSFR[REG_CCPDATIA], cpu.mSFR[REG_CCPDATIB]);
    }

private:
    static ALWAYS_INLINE void gfmBit(uint8_t &a, uint8_t &b, uint8_t &p)
    {
        if (b & 1)
            p ^= a;
        uint8_t msb = a & 0x80;
        a <<= 1;
        if (msb)
            a ^= 0x1b;
        b >>= 1;
    }

    /// GF(2^8) multiplier, using the AES polynomial
    static uint8_t galoisFieldMultiply(uint8_t a, uint8_t b)
    {
        uint8_t p = 0;
        gfmBit(a, b, p);  // 0
        gfmBit(a, b, p);  // 1
        gfmBit(a, b, p);  // 2
        gfmBit(a, b, p);  // 3
        gfmBit(a, b, p);  // 4
        gfmBit(a, b, p);  // 5
        gfmBit(a, b, p);  // 6
        gfmBit(a, b, p);  // 7
        return p;
    }
};
 

};  // namespace Cube

#endif
