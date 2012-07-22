/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBESLOTS_H
#define _CUBESLOTS_H

#include <sifteo/abi.h>
#include "macros.h"

class CubeSlot;

namespace CubeSlots {
    extern CubeSlot instances[_SYS_NUM_CUBE_SLOTS];

    /*
     * One-bit flags for each cube are packed into global vectors
     */
    extern _SYSCubeIDVector sysConnected;       /// Cube connected by the system
    extern _SYSCubeIDVector userConnected;      /// Cube is seen as connected by userspace
    extern _SYSCubeIDVector flashResetWait;     /// We need to reset flash before writing to it
    extern _SYSCubeIDVector flashResetSent;     /// We've sent an unacknowledged flash reset    
    extern _SYSCubeIDVector flashAddrPending;   /// Need to send an addressing command to the flash codec
    
    extern _SYSCubeID minUserCubes;             /// Curent cube range for userspace
    extern _SYSCubeID maxUserCubes;
    
    /*
     * Shared asset loader, for all cubes. This pointer itself must be
     * validated when it's set, but there is no guarantee of valididty for
     * individual _SYSAssetLoaderCubes.
     */
    extern _SYSAssetLoader *assetLoader;

    /*
     * In simulation only: We can opt to bypass the actual asset loader, and
     * instead decompress loadstream data directly into cube flash.
     */
#ifdef SIFTEO_SIMULATOR
    extern bool simAssetLoaderBypass;
#endif

    static ALWAYS_INLINE bool validID(_SYSCubeID id) {
        // For security/reliability, all cube IDs from game code must be checked
        return id < _SYS_NUM_CUBE_SLOTS;
    }

    static ALWAYS_INLINE _SYSCubeIDVector truncateVector(_SYSCubeIDVector cv) {
        // For security/reliability, all cube vectors from game code must be checked
        return cv & (0xFFFFFFFF << (32 - _SYS_NUM_CUBE_SLOTS));
    }

    static _SYSCubeIDVector availableSlots() {
        return truncateVector(~(sysConnected | userConnected));
    }

    void setCubeRange(unsigned minimum, unsigned maximum);

    void paintCubes(_SYSCubeIDVector cv, bool wait=true);
    void finishCubes(_SYSCubeIDVector cv);

    void assetLoaderTask(void *);
    void fetchAssetLoaderData(_SYSAssetLoaderCube *lc);
}

#endif
