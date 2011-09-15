/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_CUBE_H
#define _SIFTEO_CUBE_H

#include <sifteo/abi.h>
#include <sifteo/machine.h>
#include "radio.h"


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

    // One-bit flags for each cube are packed into global vectors
    static _SYSCubeIDVector vecEnabled;
    static _SYSCubeIDVector flashResetWait;
    static _SYSCubeIDVector flashResetSent;

    _SYSCubeID id() const {
	return this - &instances[0];
    }

    _SYSCubeIDVector bit() const {
	return 1 << id();
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

    void loadAssets(_SYSAssetGroup *a);

    static bool validID(_SYSCubeID id) {
	// For security/reliability, all cube IDs from game code must be checked
	return id < _SYS_NUM_CUBE_SLOTS;
    }
    
    static _SYSCubeIDVector truncateVector(_SYSCubeIDVector cv) {
	// For security/reliability, all cube vectors from game code must be checked
	return cv & (_SYSCubeIDVector)((1ULL << _SYS_NUM_CUBE_SLOTS) - 1);
    }

    static void enableCubes(_SYSCubeIDVector cv) {
	Sifteo::Atomic::Or(vecEnabled, cv);
    }

    static void disableCubes(_SYSCubeIDVector cv) {
	Sifteo::Atomic::And(vecEnabled, ~cv);
    }

 private:
    /// Data buffers, provided by game code
    _SYSAssetGroup *loadGroup;
    _SYSVideoBuffer *vbuf;

    /// State of flash loader
    uint8_t loadPrevACK;
    uint8_t loadBufferAvail;
};


#endif
