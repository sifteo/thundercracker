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

#ifndef _CUBESLOTS_H
#define _CUBESLOTS_H

#include <sifteo/abi.h>
#include "macros.h"
#include "bits.h"
#include "flash_syslfs.h"

class CubeSlot;

namespace CubeSlots {
    extern CubeSlot instances[_SYS_NUM_CUBE_SLOTS];

    /*
     * One-bit flags for each cube are packed into global vectors
     */
    extern _SYSCubeIDVector sysConnected;       /// Cube connected by the system
    extern _SYSCubeIDVector disconnectFlag;     /// Cube has been disconnected since last event dispatch
    extern _SYSCubeIDVector userConnected;      /// Cube is seen as connected by userspace
    extern _SYSCubeIDVector sendShutdown;       /// Sending a shutdown command to these cubes
    extern _SYSCubeIDVector sendStipple;        /// Sending a stipple pattern to these cubes
    extern _SYSCubeIDVector vramPaused;         /// Pause transmission of VRAM updates
    extern _SYSCubeIDVector touch;              /// Stretched touch-detection bits for each cube
    extern _SYSCubeIDVector waitingOnCubes;     /// Cubes that we're blocking on. Boosts radio service rate.
    extern _SYSCubeIDVector pendingHop;         /// Cubes that need a channel hop

    extern BitVector<SysLFS::NUM_PAIRINGS> pairConnected;   /// Connected cubes, indexed by pairing ID
    
    extern _SYSCubeID minUserCubes;             /// Curent cube range for userspace
    extern _SYSCubeID maxUserCubes;

    static ALWAYS_INLINE bool validID(_SYSCubeID id) {
        // For security/reliability, all cube IDs from game code must be checked
        return id < _SYS_NUM_CUBE_SLOTS;
    }

    static ALWAYS_INLINE _SYSCubeIDVector truncateVector(_SYSCubeIDVector cv) {
        // For security/reliability, all cube vectors from game code must be checked
        return cv & (0xFFFFFFFF << (32 - _SYS_NUM_CUBE_SLOTS));
    }

    static ALWAYS_INLINE uint32_t numConnected() {
        return Intrinsic::POPCOUNT(sysConnected);
    }

    static ALWAYS_INLINE bool belowCubeRange() {
        return numConnected() < minUserCubes;
    }

    static ALWAYS_INLINE bool connectionSlotsAvailable() {
        return numConnected() < maxUserCubes;
    }

    void setCubeRange(unsigned minimum, unsigned maximum);

    void paintCubes(_SYSCubeIDVector cv, bool wait=true, uint32_t excludedTasks=0);
    void finishCubes(_SYSCubeIDVector cv, uint32_t excludedTasks=0);
    void refreshCubes(_SYSCubeIDVector cv);
    void clearTouchEvents();
    void disconnectCubes(_SYSCubeIDVector cv);
}

#endif
