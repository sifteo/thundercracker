/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "event.h"
#include "cube.h"
#include "neighbors.h"

#include <sifteo/abi.h>

using namespace Sifteo;

bool Event::dispatchInProgress;
uint32_t Event::pending;
Event::VectorInfo Event::vectors[_SYS_NUM_VECTORS];


void Event::dispatch()
{
    /*
     * Skip event dispatch if we're already in an event handler
     */

    if (dispatchInProgress)
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
