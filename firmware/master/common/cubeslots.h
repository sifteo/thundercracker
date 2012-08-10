/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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

    static ALWAYS_INLINE bool belowCubeRange() {
        return Intrinsic::POPCOUNT(CubeSlots::sysConnected) < CubeSlots::minUserCubes;
    }

    void setCubeRange(unsigned minimum, unsigned maximum);

    void paintCubes(_SYSCubeIDVector cv, bool wait=true, uint32_t excludedTasks=0);
    void finishCubes(_SYSCubeIDVector cv, uint32_t excludedTasks=0);
    void refreshCubes(_SYSCubeIDVector cv);
    void clearTouchEvents();
    void disconnectCubes(_SYSCubeIDVector cv);
}

#endif
