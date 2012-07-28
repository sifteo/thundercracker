/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
