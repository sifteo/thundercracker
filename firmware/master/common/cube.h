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
#include "cubeslots.h"
#include "systime.h"

//#include "cubecodec.h"

#ifndef USE_MOCK_CUBE_CODEC
  #include "cubecodec.h"
#else
  #include "mockcubecodec.h"
  #define CubeCodec MockCubeCodec
#endif


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

    _SYSCubeID id() const {
        _SYSCubeID i = this - &CubeSlots::instances[0];
        ASSERT(i < _SYS_NUM_CUBE_SLOTS);
        STATIC_ASSERT(arraysize(CubeSlots::instances) == _SYS_NUM_CUBE_SLOTS);
        return i;
    }

    static CubeSlot &getInstance(_SYSCubeID id) {
        ASSERT(id < _SYS_NUM_CUBE_SLOTS);
        return CubeSlots::instances[id];
    }

    _SYSCubeIDVector bit() const {
        STATIC_ASSERT(_SYS_NUM_CUBE_SLOTS <= 32);
        return Sifteo::Intrinsic::LZ(id());
    }

    bool enabled() const {
        return !!(bit() & CubeSlots::vecEnabled);
    }
	
	bool connected() const {
        return !!(bit() & CubeSlots::vecConnected);
    }
	
	void setConnected() {
		CubeSlots::connectCubes(Sifteo::Intrinsic::LZ(id()));
	}
	
	void setDisconnected() {
		CubeSlots::disconnectCubes(Sifteo::Intrinsic::LZ(id()));
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

 private:
    // Limit on round-trip time
    static const unsigned RTT_DEADLINE_MS = 250;
    // number of cube (ie, not master) ticks for a neighbor tx slot.
    // represents cube bit period * num total bits in a tx sequence.
    // NOTE: must be synced with NB_BIT_TICKS * NB_TX_BITS in firmware/cube/sensors.c
    // XXX: at the moment, NB_TX_BITS is 18 on the cube, but making it wider here (24) to be conservative
    static const unsigned NEIGHBOR_TX_SLOT_TICKS = 64 * 24;
    // rollover duration of cube timer, in cube ticks
    static const unsigned NEIGHBOR_TIMER_PERIOD_TICKS = 0x1FFF; // 13 bit
    
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
