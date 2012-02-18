/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "runtime.h"
#include "cube.h"
#include "neighbors.h"
#include "tasks.h"

#include <sifteo/abi.h>

using namespace Sifteo;

jmp_buf Runtime::jmpExit;

bool Event::dispatchInProgress;
bool Event::paused = false;
uint32_t Event::pending;
Event::VectorInfo Event::vectors[_SYS_NUM_VECTORS];

void Runtime::run()
{
    if (setjmp(jmpExit))
        return;

    // siftmain;
}

void Runtime::exit()
{
    longjmp(jmpExit, 1);
}

void Event::dispatch()
{
    /*
     * Skip event dispatch if we're already in an event handler
     */

    if (dispatchInProgress || paused)
        return;
    dispatchInProgress = true;

    /*
     * Process events, by type
     */

    while (pending) {
        _SYSVectorID vid = (_SYSVectorID) Intrinsic::CLZ(pending);
        ASSERT(vid < _SYS_NUM_VECTORS);
        uint32_t vidMask = Intrinsic::LZ(vid);
        VectorInfo &vi = vectors[vid];

        // Currently all events are dispatched per-cube, even the neighbor
        // events. Loop over the Cube IDs from cubesPending.

        uint32_t cubesPending = vi.cubesPending;
	    while (cubesPending) {
            _SYSCubeID cid = (_SYSCubeID) Intrinsic::CLZ(cubesPending);
            
            // Type-specific event dispatch

            if (vidMask & _SYS_NEIGHBOR_EVENTS) {
                // Handle all neighbor events for this cube slot
                NeighborSlot::instances[cid].computeEvents();
                vidMask = _SYS_NEIGHBOR_EVENTS;
            } else {
                // Handle one normal cube event
                callCubeEvent(vid, cid);
            }

            Atomic::And(cubesPending, ~Intrinsic::LZ(cid));
        }
        vi.cubesPending = 0;
            
        Atomic::And(pending, ~vidMask);
    }

    dispatchInProgress = false;
}
