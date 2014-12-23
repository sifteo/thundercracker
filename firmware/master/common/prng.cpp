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

/*
 * Our own local pseudo-random number generator.
 *
 * We use this instead of the system PRNG, so that we can guarantee
 * the same numbers will be returned in real hardware and on all
 * simulation platforms.
 *
 * The PRNG we use is a very simple public-domain algorithm, documented here:
 * http://burtleburtle.net/bob/rand/smallprng.html
 */

#include "prng.h"
#include "systime.h"
#include <stdio.h>

void PRNG::init(_SYSPseudoRandomState *state, uint32_t seed)
{
    state->a = 0xf1ea5eed;
    state->b = state->c = state->d = seed;

    // Thoroughly mixed...
    for (uint32_t i = 0; i < 20; ++i) {
        (void)PRNG::value(state);
    }
}

uint32_t PRNG::value(_SYSPseudoRandomState *state)
{
    return valueInline(state);
}

uint32_t PRNG::valueBounded(_SYSPseudoRandomState *state, uint32_t limit)
{
    /*
     * We have to be careful to maintain the uniformity of the random number generator,
     * and to make it possible to hit every integer from 0 to limit. The easiest way to
     * do this seems to be a two-pass approach:
     *
     *  1. Discard bits to get us within a power of two of the proper limit. This does not
     *     affect the generator's uniformity or its ability to generate every possible
     *     bit pattern.
     *
     *  2. Discard individual values that are above the limit. We need to be careful here
     *     to avoid looping infinitely in case the PRNG was improperly initialized.
     */

    uint32_t mask = 0xFFFFFFFF;

    if (limit == 0) {
        // Degenerate case (would loop infinitely below)
        return 0;
    }

    if (limit == 0xFFFFFFFF) {
        // Another degenerate case. Would cause integer overflow in the fall-back below.
        return value(state);
    }

    // Figure out how many bits we can discard
    for (;;) {
        uint32_t nextMask = mask >> 1;
        if (nextMask < limit)
            break;
        mask = nextMask;
    }

    // Throw out values until we get a matching one
    for (unsigned i = 32; i; i--) {
        uint32_t v = value(state) & mask;
        if (v <= limit)
            return v;
    }

    // Well, that took too long. Fallback approach (not perfectly uniform!)
    return value(state) % (limit + 1);
}

uint32_t PRNG::anonymousValue()
{
    /*
     * For system use, generate an "anonymous" random number, by extracting
     * entropy only from the current time. This isn't at all secure, but it's
     * a simple and stateless way of generating random numbers for cases
     * where that's okay.
     */

    _SYSPseudoRandomState state;
    SysTime::Ticks now = SysTime::ticks();

    init(&state, uint32_t(now ^ (now >> 32)));
    return value(&state);
}

void PRNG::collectTimingEntropy(_SYSPseudoRandomState *state)
{
    /*
     * Add entropy to our PRNG using the current nanosecond timestamp.
     * Call this before sampling the PRNG to use the timing of your event
     * as an additional source of randomness.
     *
     * Unlike just re-seeding the PRNG using the nanosecond clock, this
     * preserves a portion of the existing entropy in the PRNG state.
     */

    SysTime::Ticks now = SysTime::ticks();
    init(state, value(state) ^ uint32_t(now ^ (now >> 32)));
}
