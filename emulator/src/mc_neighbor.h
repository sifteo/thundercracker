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

#ifndef _MC_NEIGHBOR_H
#define _MC_NEIGHBOR_H

/*
 * Simulated neighbor transmit for the MC.
 * This file contains simulation-only definitions.
 *
 * This does NOT run on the normal MC simulation thread! Updates come in from the UI
 * thread, and we run the hardware simulation on the cube thread, as we need to run our
 * transmit process in lockstep with the cube clock. As such, in a lot of ways this file
 * actually acts much more like it's part of the cube hardware model than the MC.
 */

#include "macros.h"
#include "vtime.h"
#include "neighbor_tx.h"

class MCNeighbor {
public:

    static void updateNeighbor(bool touching, unsigned mcSide, unsigned cube, unsigned cubeSide);

    static void cubeInit(const VirtualTime *vtime) {
        deadline.init(vtime);
    }

    static ALWAYS_INLINE void cubeTick() {
        if (deadline.hasPassed())
            deadlineWork();
    }

    static ALWAYS_INLINE uint64_t cubeDeadlineRemaining() {
        return deadline.remaining();
    }

private:

    friend class NeighborTX;

    static const unsigned NUM_SIDES = 2;

    struct CubeInfo {
        unsigned id;
        unsigned side;
    };

    static CubeInfo cubes[NUM_SIDES];

    // Cube thread, virtual time tracking
    static TickDeadline deadline;
    static uint32_t txRegister;

    // Atomic words
    static uint32_t nbrSides;
    static uint32_t txSides;
    static uint32_t txData;

    static void deadlineWork();
    static void transmitPulse(const CubeInfo &dest);
};

#endif
