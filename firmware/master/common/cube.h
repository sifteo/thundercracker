/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBE_H
#define _CUBE_H

#include <sifteo/abi.h>
#include "machine.h"
#include "macros.h"
#include "radio.h"
#include "svmmemory.h"
#include "cubeslots.h"
#include "systime.h"
#include "cubecodec.h"
#include "paintcontrol.h"
#include "flash_syslfs.h"
#include "motion.h"


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

    // System connect/disconnect handlers (ISR context)
    void connect(SysLFS::Key cubeRecord, const RadioAddress &addr, const RF_ACKType &fullACK);
    void disconnect();

    // Userspace connect/disconnect handlers (Task context)
    void userConnect();
    void userDisconnect();

    _SYSCubeID id() const {
        _SYSCubeID i = this - &CubeSlots::instances[0];
        ASSERT(i < _SYS_NUM_CUBE_SLOTS);
        STATIC_ASSERT(arraysize(CubeSlots::instances) == _SYS_NUM_CUBE_SLOTS);
        return i;
    }

    static ALWAYS_INLINE CubeSlot& getInstance(_SYSCubeID id) {
        ASSERT(id < _SYS_NUM_CUBE_SLOTS);
        return CubeSlots::instances[id];
    }

    _SYSCubeIDVector ALWAYS_INLINE bit() const {
        STATIC_ASSERT(_SYS_NUM_CUBE_SLOTS <= 32);
        return Intrinsic::LZ(id());
    }

    bool ALWAYS_INLINE isSysConnected() const {
        return !!(bit() & CubeSlots::sysConnected);
    }

    bool ALWAYS_INLINE isUserConnected() const {
        return !!(bit() & CubeSlots::userConnected);
    }

    bool isTouching() const;
    void clearTouchEvent() const;

    void ALWAYS_INLINE setVideoBuffer(_SYSVideoBuffer *v) {
        vbuf = v;
    }

    void ALWAYS_INLINE setMotionBuffer(_SYSMotionBuffer *m) {
        motionWriter.setBuffer(m);
    }

    ALWAYS_INLINE const _SYSByte4 getAccelState() {
        return MotionUtil::captureAccelState(lastACK);
    }

    ALWAYS_INLINE const uint8_t* getRawNeighbors() const {
        return lastACK.neighbors;
    }

    _SYSAssetGroupCube *assetGroupCube(struct _SYSAssetGroup *group) {
        _SYSAssetGroupCube *cube = reinterpret_cast<_SYSAssetGroupCube*>(group + 1) + id();
        if (SvmMemory::mapRAM(cube, sizeof *cube))
            return cube;
        else
            return 0;
    }

    _SYSAssetLoaderCube *assetLoaderCube(struct _SYSAssetLoader *loader) {
        _SYSAssetLoaderCube *cube = reinterpret_cast<_SYSAssetLoaderCube*>(loader + 1) + id();
        if (SvmMemory::mapRAM(cube, sizeof *cube))
            return cube;
        else
            return 0;
    }

    bool isAssetLoading(_SYSAssetLoader *L) const {
        // Caller-provided SYSAssetLoader, to support cases when we
        // must read "assetLoader" exactly once.
        return L && (L->cubeVec & ~L->complete & bit());
    }

    bool isAssetLoading() const {
        return isAssetLoading(CubeSlots::assetLoader);
    }

    void startAssetLoad(SvmMemory::VirtAddr groupVA, uint16_t baseAddr);

    void beginFinish() {
        if (vbuf)
            return paintControl.beginFinish(this);
    }

    bool pollForFinish(SysTime::Ticks now) {
        // Finish is only meaningful when we still have a vbuf attached.
        // Returns 'true' if we're finished.

        if (vbuf)
            return paintControl.pollForFinish(this, now);
        else
            return true;
    }

    ALWAYS_INLINE void waitForPaint(uint32_t excludedTasks) {
        paintControl.waitForPaint(this, excludedTasks);
    }

    ALWAYS_INLINE void triggerPaint(SysTime::Ticks now) {
        // Allow continuous rendering only when not loading assets
        paintControl.triggerPaint(this, now);
    }

    uint64_t getHWID();

    unsigned ALWAYS_INLINE getRawBatteryV() const {
        return lastACK.battery_v;
    }

    uint8_t ALWAYS_INLINE getLastFrameACK() const {
        return lastACK.frame_count;
    }

    ALWAYS_INLINE _SYSVideoBuffer* getVBuf() const {
        return vbuf;
    }

    ALWAYS_INLINE const RadioAddress *getRadioAddress() const {
        return &address;
    }

    ALWAYS_INLINE SysLFS::Key getCubeRecordKey() const {
        ASSERT(cubeRecord >= SysLFS::kCubeBase);
        ASSERT(cubeRecord < SysLFS::kCubeBase + SysLFS::kCubeCount);
        return cubeRecord;
    }

 private:
    // Limit on round-trip time
    static const unsigned RTT_DEADLINE_MS = 250;

    // Large data
    SysTime::Ticks flashDeadline;
    PaintControl paintControl;

    // Other aligned data
    _SYSVideoBuffer *vbuf;
    MotionWriter motionWriter;
    CubeCodec codec;
    uint16_t timeSyncState;

    // Byte variables
    RadioAddress address;
    SysLFS::Key cubeRecord;
    RF_ACKType lastACK;

    DEBUG_ONLY(SysTime::Ticks assetLoadTimestamp);

    void requestFlashReset();
    uint16_t calculateTimeSync();

    void queryResponse(const PacketBuffer &packet);
};

#endif
