/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
    
    void getNeighborState(_SYSNeighborState *state) {
        *state = neighbors;
    }
    
private:
    void addNeighborToSide(_SYSCubeID id, _SYSSideID side);
    void clearSide(_SYSSideID side);
    void removeNeighborFromSide(_SYSCubeID id, _SYSSideID side);

    uint8_t prevNeighbors[4]; // this is in the format the raw neighbors -- it's more than just a cubeID
    _SYSNeighborState neighbors; // these are just cubeIDs
};




#endif
