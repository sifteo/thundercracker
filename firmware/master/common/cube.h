/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBE_H
#define _CUBE_H

#include <sifteo/abi.h>
#include <sifteo/machine.h>
#include "radio.h"
#include "runtime.h"
#include "cubecodec.h"
#include "systime.h"


/**
 * One cube, or potential cube. The firmware is built with a fixed
 * number of statically allocated cube slots. Under application
 * control, these slots can be enabled. When a slot is enabled, we try
 * to fill it with a physical cube. Once a physical cube connects, we
 * can start writing to it.
 *
 * So, a slot only needs to track some very basic cube state. Slots
 * are intended to be very lightweight, with any larger buffers
 * allocated and attached to the slot by application code.
 */

class CubeSlot {
 public:
    bool radioProduce(PacketTransmission &tx);
    void radioAcknowledge(const PacketBuffer &packet);
    void radioTimeout();

    static CubeSlot instances[_SYS_NUM_CUBE_SLOTS];

    /*
     * One-bit flags for each cube are packed into global vectors
     */
    static _SYSCubeIDVector vecEnabled;         /// Cube enabled
    static _SYSCubeIDVector flashResetWait;     /// We need to reset flash before writing to it
    static _SYSCubeIDVector flashResetSent;     /// We've sent an unacknowledged flash reset    
    static _SYSCubeIDVector flashACKValid;      /// 'flashPrevACK' is valid
    static _SYSCubeIDVector frameACKValid;      /// 'framePrevACK' is valid
    static _SYSCubeIDVector neighborACKValid;   /// Neighbor/touch state is valid
    
    static void enableCubes(_SYSCubeIDVector cv); 
    static void disableCubes(_SYSCubeIDVector cv);

    _SYSCubeID id() const {
        _SYSCubeID i = this - &instances[0];
        ASSERT(i < _SYS_NUM_CUBE_SLOTS);
        STATIC_ASSERT(arraysize(instances) == _SYS_NUM_CUBE_SLOTS);
        return i;
    }

    static CubeSlot &getInstance(_SYSCubeID id) {
        ASSERT(id < _SYS_NUM_CUBE_SLOTS);
        return instances[id];
    }

    _SYSCubeIDVector bit() const {
        STATIC_ASSERT(_SYS_NUM_CUBE_SLOTS <= 32);
        return Sifteo::Intrinsic::LZ(id());
    }

    bool enabled() const {
        return !!(bit() & vecEnabled);
    }

    void setVideoBuffer(_SYSVideoBuffer *v) {
        vbuf = v;
    }

    bool isAssetGroupLoaded(_SYSAssetGroup *a) {
        return !!(bit() & a->doneCubes);
    }

    _SYSAssetGroup *getLastAssetGroup(void) const {
        return loadGroup;
    }

    void getAccelState(struct _SYSAccelState *state) {
        *state = accelState;
    }

    void getRawNeighbors(uint8_t buf[4]) {
        // XXX: Raw neighbor data for testing/demoing only
        buf[0] = neighbors[0];
        buf[1] = neighbors[1];
        buf[2] = neighbors[2];
        buf[3] = neighbors[3];
    }

    _SYSAssetGroupCube *assetCube(const struct _SYSAssetGroup *group) {
        /*
         * Safely return this cube's per-cube data on a particular
         * asset group.  If the user-pointer check fails, returns
         * NULL.
         */
        _SYSCubeID i = id();
        if (Runtime::checkUserPointer(group->cubes, (sizeof group->cubes[0]) * (i + 1)))
            return &group->cubes[i];
        return 0;
    }

    void loadAssets(_SYSAssetGroup *a);
    void waitForPaint();
    void waitForFinish();
    void triggerPaint(SysTime::Ticks timestamp);

    const _SYSCubeHWID & getHWID() const {
        return hwid;
    }
    
    uint16_t getRawBatteryV() const {
        return rawBatteryV;
    }
    
    static bool validID(_SYSCubeID id) {
        // For security/reliability, all cube IDs from game code must be checked
        return id < _SYS_NUM_CUBE_SLOTS;
    }
    
    static _SYSCubeIDVector truncateVector(_SYSCubeIDVector cv) {
        // For security/reliability, all cube vectors from game code must be checked
        return cv & (0xFFFFFFFF << (32 - _SYS_NUM_CUBE_SLOTS));
    }

    static void paintCubes(_SYSCubeIDVector cv);
    static void finishCubes(_SYSCubeIDVector cv);

 private:
    // Limit on round-trip time
    static const unsigned RTT_DEADLINE_MS = 250;
    
    /*
     * Data buffers, provided by game code.
     *
     * 'vbuf' is non-NULL any time this cube has a buffer attached. We
     * will try to send out any changes in that buffer. The buffer
     * pointer is guaranteed to remain valid until it's set to NULL or
     * to a different pointer, which can happen only outside of IRQ
     * context.
     *
     * The loadGroup pointer can be set only in non-IRQ context, and
     * only by application code. It represents the current group we're
     * downloading, or the last group that was sent. The 'loadGroup'
     * pointer is not automatically NULL'ed after we finish loading the
     * group, since it's used for event dispatch purposes as well.
     */

    _SYSAssetGroup *loadGroup;
    _SYSVideoBuffer *vbuf;
    RadioAddress address;
    
    DEBUG_ONLY(SysTime::Ticks assetLoadTimestamp);
    
    SysTime::Ticks paintTimestamp;      // Used only by thread
    SysTime::Ticks flashDeadline;       // Used only by ISR
    int32_t pendingFrames;
    uint32_t timeSyncState;             // XXX: For the current time-sync hack

    // Packet encoder state
    CubeCodec codec;

    // Byte variables
    uint8_t flashPrevACK;
    uint8_t framePrevACK;
    uint8_t neighbors[4];

    // Sensors
    uint16_t rawBatteryV;
    _SYSAccelState accelState;
    _SYSCubeHWID hwid;
};


#endif
