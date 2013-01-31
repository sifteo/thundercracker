/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <protocol.h>
#include "machine.h"
#include "neighbor_protocol.h"
#include "neighborslot.h"
#include "vram.h"
#include "event.h"

/**
 * Unlike generic neighbors, which only require a one-way connection before we'll
 * report them, cube-to-cube neighborings are tracked using a matrix which guarantees
 * their consistency even if our sensor data isn't perfect. This matrix is only used
 * for cubes, not for bases or other hypothetical neighborable objects.
 */
struct CubeNeighborPair {
    static const _SYSSideID NO_SIDE = -1;
    static const unsigned NUM_UNIQUE_PAIRS = _SYS_NUM_CUBE_SLOTS * (_SYS_NUM_CUBE_SLOTS-1) / 2;

    static CubeNeighborPair matrix[NUM_UNIQUE_PAIRS];

    _SYSSideID side0 : 4;
    _SYSSideID side1 : 4;

    CubeNeighborPair() : side0(NO_SIDE), side1(NO_SIDE) {}

    ALWAYS_INLINE bool fullyConnected() const {
        return side0 != NO_SIDE && side1 != NO_SIDE;
    }
    
    ALWAYS_INLINE bool fullyDisconnected() const {
        return side0 == NO_SIDE && side1 == NO_SIDE;
    }

    ALWAYS_INLINE void clear() {
        side0 = NO_SIDE;
        side1 = NO_SIDE;
    }

    _SYSSideID setSideAndGetOtherSide(_SYSCubeID cid0, _SYSCubeID cid1, _SYSSideID side, CubeNeighborPair **outPair)
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

    CubeNeighborPair* lookup(_SYSCubeID cid0, _SYSCubeID cid1)
    {
        // invariant this == pairs[0]
        // invariant cid0 < cid1
        const unsigned n = _SYS_NUM_CUBE_SLOTS - cid0;
        return this + ( (NUM_UNIQUE_PAIRS-1) - ( (n*(n-1)) >> 1 ) + cid1 );
    }
};


NeighborSlot NeighborSlot::instances[_SYS_NUM_CUBE_SLOTS];
CubeNeighborPair CubeNeighborPair::matrix[CubeNeighborPair::NUM_UNIQUE_PAIRS];


bool NeighborSlot::sendNextEvent()
{
    /*
     * Send at most one neighbor event. If we can dispatch an event, we
     * return 'true', and Event will call us back later to try and get
     * another event. If no events can be disptached, returns 'false'.
     */

     if (CubeSlots::instances[id()].isSysConnected()) {

        const uint8_t *rawNeighbors = CubeSlots::instances[id()].getRawNeighbors();

        for (_SYSSideID side=0; side<4; ++side) {
            _SYSNeighborID prev = hardwareNeighborToABI(prevNeighbors[side]);
            _SYSNeighborID next = hardwareNeighborToABI(rawNeighbors[side]);

            // filter out zombies
            if (next < _SYS_NUM_CUBE_SLOTS && !CubeSlots::instances[next].isSysConnected()) {
                next = _SYS_NEIGHBOR_NONE;
            }

            if (prev != next) {
                // Neighbor changed

                if (prev != _SYS_NEIGHBOR_NONE && removeNeighborFromSide(prev, side))
                    return true;

                if (next != _SYS_NEIGHBOR_NONE && addNeighborToSide(next, side))
                    return true;
            }

            // If we made it this far, commit the state to memory.
            if (next == _SYS_NEIGHBOR_NONE) {
                prevNeighbors[side] = next; 
            } else {
                prevNeighbors[side] = rawNeighbors[side];
            }
        }
    
    } else {

        for(_SYSSideID side=0; side<4; ++side) {
            _SYSNeighborID prev = neighbors.sides[side];
            if (prev != _SYS_NEIGHBOR_NONE) {
                // kill zombie neighbor
                if (removeNeighborFromSide(prev, side)) {
                    return true;
                }
            }
        }

        doResetSlot();
    }

    // No more events
    return false;
}

unsigned NeighborSlot::hardwareNeighborToABI(unsigned byte)
{
    /*
     * Convert the encoding used for hardware neighbors (in our ACK buffer) to the
     * encoding used by our SDK. This ignores non-neighbor bits.
     *
     * We squash all base IDs to the same value. At the physical layer, our base
     * IDs will be continually shifting as the CubeConnector rotates between
     * pairing channels. Userspace shouldn't care about this, and we want
     * all base IDs to be represented by _SYS_NEIGHBOR_BASE.
     *
     * The userspace ABI has provisions for multiple bases and/or base sides,
     * but currently we don't bother going to the extra trouble to figure out
     * which side of the base a cube is neighbored to. (This would likely
     * be an iterative procedure of narrowing down the side by transmitting
     * different patterns on each)
     */

    if (byte == _SYS_NEIGHBOR_NONE) {
        return _SYS_NEIGHBOR_NONE;
    }

    if (byte & 0x80) {
        uint8_t id = byte & 0x1F;

        if (id < _SYS_NUM_CUBE_SLOTS)
            return id;

        return _SYS_NEIGHBOR_BASE;
    }

    // No neighbor present
    return _SYS_NEIGHBOR_NONE;
}

void NeighborSlot::resetSlots(_SYSCubeIDVector cv)
{
    while (cv) {
        _SYSCubeID cubeId = Intrinsic::CLZ(cv);
        instances[cubeId].doResetSlot();
        cv ^= Intrinsic::LZ(cubeId);
    }
}

void NeighborSlot::resetPairs() {
    int c = id();
    c = c * _SYS_NUM_CUBE_SLOTS - ((c*(c-1))>>1) - 1;
    for(int i=0; i<int(CubeNeighborPair::NUM_UNIQUE_PAIRS); ++i) {
        if (c - i > c) {
            CubeNeighborPair::matrix[i].clear();
        }
    }
}

void NeighborSlot::resetAllPairs()
{
    for (CubeNeighborPair *p = CubeNeighborPair::matrix;
        p != CubeNeighborPair::matrix + CubeNeighborPair::NUM_UNIQUE_PAIRS; ++p) {
        p->clear();
    }
}

void NeighborSlot::doResetSlot() {
    memset(neighbors.sides, _SYS_NEIGHBOR_NONE, sizeof neighbors);
    memset(prevNeighbors, 0x00, sizeof prevNeighbors);
}

bool NeighborSlot::addNeighborToSide(_SYSNeighborID dstId, _SYSSideID side)
{
    bool dstIsCube = dstId < _SYS_NUM_CUBE_SLOTS;
    _SYSSideID dstSide = 0;

    if (dstIsCube) {
        // Update cube neighbor matrix
        CubeNeighborPair* pair;
        dstSide = CubeNeighborPair::matrix->setSideAndGetOtherSide(id(), dstId, side, &pair);
        if (!pair->fullyConnected())
            return false;
    }

    if (neighbors.sides[side] != dstId) {
        if (clearSide(side))
            return true;

        if (dstIsCube && instances[dstId].clearSide(dstSide))
            return true;

        neighbors.sides[side] = dstId;
        if (dstIsCube)
            instances[dstId].neighbors.sides[dstSide] = id();

        if (Event::callNeighborEvent(_SYS_NEIGHBOR_ADD, id(), side, dstId, dstSide))
            return true;
    }
    
    return false;
}

bool NeighborSlot::clearSide(_SYSSideID side)
{
    /*
     * Send a 'remove' event for anyone who's neighbored with "side".
     * Sends at most one event.
     */

    _SYSNeighborID otherId = neighbors.sides[side];
    neighbors.sides[side] = _SYS_NEIGHBOR_NONE;

    if (otherId == _SYS_NEIGHBOR_NONE)
        return false;

    if (otherId < _SYS_NUM_CUBE_SLOTS) {
        // De-neighboring with another cube. Use the neighbor matrix

        for (_SYSSideID otherSide=0; otherSide<4; ++otherSide) {
            if (instances[otherId].neighbors.sides[otherSide] == id()) {
                instances[otherId].neighbors.sides[otherSide] = _SYS_NEIGHBOR_NONE;

                Event::callNeighborEvent(_SYS_NEIGHBOR_REMOVE, id(), side, otherId, otherSide);
                return true;
            }
        }

    } else {
        // De-neighboring a non-cube entity. Skip the matrix, send a raw event with no side information.

        Event::callNeighborEvent(_SYS_NEIGHBOR_REMOVE, id(), side, otherId, 0);
        return true;
    }

    return false;
}

bool NeighborSlot::removeNeighborFromSide(_SYSNeighborID dstId, _SYSSideID side)
{
    if (dstId < _SYS_NUM_CUBE_SLOTS) {
        // Update cube neighbor matrix
        CubeNeighborPair* pair;
        CubeNeighborPair::matrix->setSideAndGetOtherSide(id(), dstId, CubeNeighborPair::NO_SIDE, &pair);
        if (!pair->fullyDisconnected())
            return false;
    }

    if (neighbors.sides[side] == dstId)
        return clearSide(side);

    return false;
}
