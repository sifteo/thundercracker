/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _NEIGHBORSLOT_H__
#define _NEIGHBORSLOT_H__

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
    
    bool sendNextEvent();

    static void resetSlots(_SYSCubeIDVector cv);
    static void resetPairs(_SYSCubeIDVector cv);
    
    const _SYSNeighborState &getNeighborState() {
        return neighbors;
    }
    
private:
    static unsigned hardwareNeighborToABI(unsigned byte);

    bool addNeighborToSide(_SYSNeighborID id, _SYSSideID side);
    bool clearSide(_SYSSideID side);
    bool removeNeighborFromSide(_SYSNeighborID id, _SYSSideID side);

    uint8_t prevNeighbors[4];       // in the raw RF ACK format
    _SYSNeighborState neighbors;    // these are ABI NeighborIDs.
};


#endif
