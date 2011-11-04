/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _NEIGHBORS_H__
#define _NEIGHBORS_H__

#include "cube.h"

/* 
 * Neighbor Coalescing
 * 
 * Neighbors are not created until both cubes report the pair
 *      This way, application code can rely on all neighboring relationships
 *      being symmetric.
 * 
 * Neighbors are not removed until both cubes report the unpairing
 *      This reduces the amount of neighboring event noise coming from the 
 *      firmware.
 * 
 * gCoalescedPairs stores a global (cube X cube) -> (side X side) maps
 * 
 * coalescedNeighbors stores the "corrected" neighboring state, which should
 *      be used by the application, rather than the raw neighbors.
 */

class NeighborSlot {
public:
    static NeighborSlot instances[_SYS_NUM_CUBE_SLOTS];
    
    _SYSCubeID id() const {
        _SYSCubeID i = this - &instances[0];
        ASSERT(i < _SYS_NUM_CUBE_SLOTS);
        STATIC_ASSERT(arraysize(instances) == _SYS_NUM_CUBE_SLOTS);
        return i;
    }
    
    void computeEvents();
    static void resetSlots(_SYSCubeIDVector cv);
    static void resetPairs(_SYSCubeIDVector cv);
    
    void getCoalescedNeighbors(_SYSCubeID buf[4]) {
        buf[0] = neighbors[0];
        buf[1] = neighbors[1];
        buf[2] = neighbors[2];
        buf[3] = neighbors[3];
    }
    
private:
    void addNeighborToSide(_SYSCubeID id, _SYSSideID side);
    void clearSide(_SYSSideID side);
    void removeNeighborFromSide(_SYSCubeID id, _SYSSideID side);

    uint8_t prevNeighbors[4];
    _SYSCubeID neighbors[4];
};




#endif
