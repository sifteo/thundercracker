/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
