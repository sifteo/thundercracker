/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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

#define rot(x,k) (((x)<<(k))|((x)>>(32-(k))))

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
    uint32_t e = state->a - rot(state->b, 27);
    state->a = state->b ^ rot(state->c, 17);
    state->b = state->c + state->d;
    state->c = state->d + e;
    state->d = e + state->a;
    return state->d;
}

#include <stdio.h>

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
