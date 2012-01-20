/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _PRNG_H
#define _PRNG_H

#include <sifteo/abi.h>

struct PRNG {
    static void init(_SYSPseudoRandomState *state, uint32_t seed);
    static uint32_t value(_SYSPseudoRandomState *state);
    static uint32_t valueBounded(_SYSPseudoRandomState *state, uint32_t limit);
};

#endif
