/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
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

#ifndef _PRNG_H
#define _PRNG_H

#include <sifteo/abi.h>
#include "machine.h"


struct PRNG {
    // Stateful userspace-accessible PRNG
    static void init(_SYSPseudoRandomState *state, uint32_t seed);
    static uint32_t value(_SYSPseudoRandomState *state);
    static uint32_t valueBounded(_SYSPseudoRandomState *state, uint32_t limit);

    static ALWAYS_INLINE uint32_t valueInline(_SYSPseudoRandomState *state)
    {
        uint32_t e = state->a - Intrinsic::ROL(state->b, 27);
        state->a = state->b ^ Intrinsic::ROL(state->c, 17);
        state->b = state->c + state->d;
        state->c = state->d + e;
        state->d = e + state->a;
        return state->d;
    }

    // For internal use by the system
    static uint32_t anonymousValue();

    static void collectTimingEntropy(_SYSPseudoRandomState *state);
};

#endif
