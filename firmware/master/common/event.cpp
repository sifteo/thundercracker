/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "event.h"
#include "cube.h"
#include "neighbors.h"

#include <sifteo/abi.h>

using namespace Sifteo;

uint32_t Event::pending;
Event::VectorInfo Event::vectors[_SYS_NUM_VECTORS];


void Event::dispatch()
{
    /*
     * Find the next event that needs to be dispatched, if any, and
     * dispatch it. In order to avoid allocating an unbounded number of
     * user-mode stack frames, this function is limited to dispatching
     * at most one event at a time.
     */

    if (!SvmRuntime::canSendEvent())
        return;

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

	    while (vi.cubesPending) {
            _SYSCubeID cid = (_SYSCubeID) Intrinsic::CLZ(vi.cubesPending);
            Atomic::And(vi.cubesPending, ~Intrinsic::LZ(cid));

            // Type-specific event dispatch

            if (vidMask & _SYS_NEIGHBOR_EVENTS) {
                // Handle all neighbor events for this cube slot
                // XXX: Need to make Neighbor code able to dispatch single events at a time.
                //NeighborSlot::instances[cid].computeEvents();
                vidMask = _SYS_NEIGHBOR_EVENTS;
            } else {
                // Handle one normal cube event
                if (callCubeEvent(vid, cid));
                    return;
            }
        }

        // Once all cubesPending bits are clear, clear the vector bit.
        Atomic::And(pending, ~vidMask);
    }
}
