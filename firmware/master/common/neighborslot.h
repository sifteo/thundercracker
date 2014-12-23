/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
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
    static void resetAllPairs();
    void resetPairs();
    
    const _SYSNeighborState &getNeighborState() {
        return neighbors;
    }
    
private:
    static unsigned hardwareNeighborToABI(unsigned byte);

    bool addNeighborToSide(_SYSNeighborID id, _SYSSideID side);
    bool clearSide(_SYSSideID side);
    bool removeNeighborFromSide(_SYSNeighborID id, _SYSSideID side);

    void doResetSlot();

    uint8_t prevNeighbors[4];       // in the raw RF ACK format
    _SYSNeighborState neighbors;    // these are ABI NeighborIDs.
};


#endif
