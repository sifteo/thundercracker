/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBESLOTS_H
#define _CUBESLOTS_H

#include <sifteo/abi.h>

#ifdef USE_MOCK_CUBE
  #define CubeSlot MockCubeSlot
#endif

class CubeSlot;
struct PacketBuffer;

namespace CubeSlots {
    extern CubeSlot instances[_SYS_NUM_CUBE_SLOTS];
    
    /*
     * One-bit flags for each cube are packed into global vectors
     */
    extern _SYSCubeIDVector vecEnabled;         /// Cube enabled
	extern _SYSCubeIDVector vecConnected;       /// Cube connected
    extern _SYSCubeIDVector flashResetWait;     /// We need to reset flash before writing to it
    extern _SYSCubeIDVector flashResetSent;     /// We've sent an unacknowledged flash reset    
    extern _SYSCubeIDVector flashACKValid;      /// 'flashPrevACK' is valid
    extern _SYSCubeIDVector frameACKValid;      /// 'framePrevACK' is valid
    extern _SYSCubeIDVector neighborACKValid;   /// Neighbor/touch state is valid
    extern _SYSCubeIDVector expectStaleACK;     /// Expect this cube to send us one stale (buffered) ACK
    
    extern _SYSCubeID minCubes;
    extern _SYSCubeID maxCubes;
    
    static bool validID(_SYSCubeID id) {
        // For security/reliability, all cube IDs from game code must be checked
        return id < _SYS_NUM_CUBE_SLOTS;
    }
    
    void enableCubes(_SYSCubeIDVector cv); 
    void disableCubes(_SYSCubeIDVector cv);
    void solicitCubes(_SYSCubeID min, _SYSCubeID max);
    void connectCubes(_SYSCubeIDVector cv);
    void disconnectCubes(_SYSCubeIDVector cv);
    
    static _SYSCubeIDVector truncateVector(_SYSCubeIDVector cv) {
        // For security/reliability, all cube vectors from game code must be checked
        return cv & (0xFFFFFFFF << (32 - _SYS_NUM_CUBE_SLOTS));
    }
    
    void paintCubes(_SYSCubeIDVector cv);
    void finishCubes(_SYSCubeIDVector cv);
}

#endif
