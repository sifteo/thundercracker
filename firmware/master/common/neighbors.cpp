/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <protocol.h>
#include "machine.h"
#include "neighbors.h"
#include "vram.h"
#include "event.h"

#define CUBE_ID_MASK        (0x1F)
#define HAS_NEIGHBOR_MASK   (0x80)
#define FULL_MASK           (CUBE_ID_MASK | HAS_NEIGHBOR_MASK)
#define NO_SIDE             (-1)
#define NUM_UNIQUE_PAIRS    ((_SYS_NUM_CUBE_SLOTS*(_SYS_NUM_CUBE_SLOTS-1)) >> 1)

NeighborSlot NeighborSlot::instances[_SYS_NUM_CUBE_SLOTS];


struct NeighborPair {
    _SYSSideID side0 : 4;
    _SYSSideID side1 : 4;
    inline bool fullyConnected() const { return side0 != NO_SIDE && side1 != NO_SIDE; }
    inline bool fullyDisconnected() const { return side0 == NO_SIDE && side1 == NO_SIDE; }
    inline void clear() { side0=NO_SIDE; side1=NO_SIDE; }
    NeighborPair() : side0(NO_SIDE), side1(NO_SIDE) {}

    _SYSSideID setSideAndGetOtherSide(_SYSCubeID cid0, _SYSCubeID cid1,
        _SYSSideID side, NeighborPair** outPair)
    {
        // abstract the order-of-arguments invariant of lookup()
        if (cid0 < cid1) {
            *outPair = lookup(cid0, cid1);
            (*outPair)->side0 = side;
            return (*outPair)->side1;
        } else {
            *outPair = lookup(cid1, cid0);
            (*outPair)->side1 = side;
            return (*outPair)->side0;
        }
    }

    inline NeighborPair* lookup(_SYSCubeID cid0, _SYSCubeID cid1)
    {
        // invariant this == pairs[0]
        // invariant cid0 < cid1
        const unsigned n = _SYS_NUM_CUBE_SLOTS - cid0;
        return this + ( (NUM_UNIQUE_PAIRS-1) - ( (n*(n-1)) >> 1 ) + cid1 );
    }
};

static NeighborPair gCubesToSides[NUM_UNIQUE_PAIRS];


bool NeighborSlot::sendNextEvent()
{
    /*
     * Send at most one neighbor event. If we can dispatch an event, we
     * return 'true', and Event will call us back later to try and get
     * another event. If no events can be disptached, returns 'false'.
     */

    const uint8_t *rawNeighbors = CubeSlots::instances[id()].getRawNeighbors();

    for (_SYSSideID side=0; side<4; ++side) {
        uint8_t prevNeighbor = prevNeighbors[side];
        uint8_t rawNeighbor = rawNeighbors[side];

        if (prevNeighbor & HAS_NEIGHBOR_MASK) {
            // Used to be neighbored. If we're no longer neighbored, OR if we're
            // now neighbored to a different cube, start out by processing a Remove,
            // then do an Add.

            if ((rawNeighbor & HAS_NEIGHBOR_MASK) == 0) {
                if (removeNeighborFromSide(prevNeighbor & CUBE_ID_MASK, side))
                    return true;

            } else if ((prevNeighbor & CUBE_ID_MASK) != (rawNeighbor & CUBE_ID_MASK)) {
                if (removeNeighborFromSide(prevNeighbor & CUBE_ID_MASK, side))
                    return true;
                if (addNeighborToSide(rawNeighbors[side] & CUBE_ID_MASK, side))
                    return true;
            }

        } else if (rawNeighbors[side] & HAS_NEIGHBOR_MASK) {
            // Adding a new neighbor.

            if (addNeighborToSide(rawNeighbors[side] & CUBE_ID_MASK, side))
                return true;
        }

        // If we made it this far, commit the state to memory.
        prevNeighbors[side] = rawNeighbors[side];
    }

    // No more events
    return false;
}

void NeighborSlot::resetSlots(_SYSCubeIDVector cv) {
    while (cv) {
        _SYSCubeID cubeId = Intrinsic::CLZ(cv);
        memset(instances[cubeId].neighbors.sides, 0xff, sizeof instances[cubeId].neighbors);
        memset(instances[cubeId].prevNeighbors, 0x00, sizeof instances[cubeId].prevNeighbors);
        cv ^= Intrinsic::LZ(cubeId);
    }
}

void NeighborSlot::resetPairs(_SYSCubeIDVector cv) {
    for (NeighborPair *p = gCubesToSides; p!= gCubesToSides+NUM_UNIQUE_PAIRS; ++p) {
        p->clear();
    }
}

bool NeighborSlot::addNeighborToSide(_SYSCubeID dstId, _SYSSideID side) {
    NeighborPair* pair;
    _SYSSideID dstSide = gCubesToSides->setSideAndGetOtherSide(id(), dstId, side, &pair);

    if (pair->fullyConnected() && neighbors.sides[side] != dstId) {
        if (clearSide(side))
            return true;
        if (instances[dstId].clearSide(dstSide))
            return true;

        neighbors.sides[side] = dstId;
        instances[dstId].neighbors.sides[dstSide] = id();

        if (Event::callNeighborEvent(_SYS_NEIGHBOR_ADD, id(), side, dstId, dstSide))
            return true;
    }
    
    return false;
}

bool NeighborSlot::clearSide(_SYSSideID side) {
    // Send a 'remove' event for anyone who's neighbored with "side".
    // Sends at most one event.

    _SYSCubeID otherId = neighbors.sides[side];
    neighbors.sides[side] = 0xff;
    if (otherId == 0xff)
        return false;

    for (_SYSSideID otherSide=0; otherSide<4; ++otherSide) {
        if (instances[otherId].neighbors.sides[otherSide] == id()) {
            instances[otherId].neighbors.sides[otherSide] = 0xff;

            Event::callNeighborEvent(_SYS_NEIGHBOR_REMOVE, id(), side, otherId, otherSide);
            return true;
        }
    }

    return false;
}

bool NeighborSlot::removeNeighborFromSide(_SYSCubeID dstId, _SYSSideID side) {
    NeighborPair* pair;
    gCubesToSides->setSideAndGetOtherSide(id(), dstId, NO_SIDE, &pair);
    if (pair->fullyDisconnected() && neighbors.sides[side] == dstId)
        return clearSide(side);
    return false;
}
