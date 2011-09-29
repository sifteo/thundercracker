/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_RUNTIME_H
#define _SIFTEO_RUNTIME_H

#include <setjmp.h>
#include <sifteo/abi.h>
#include <sifteo/machine.h>


/**
 * Game code runtime environment
 */

class Runtime {
 public:
    static void run();
    static void exit();

    static bool checkUserPointer(const void *ptr, intptr_t size, bool allowNULL=false) {
        /*
         * XXX: Validate a memory address that was provided to us by the game,
         *      make sure it isn't outside the game's sandbox region. This code
         *      MUST use overflow-safe arithmetic!
         *
         * Also checks for NULL pointers, assuming allowNULL isn't true.
         *
         * May be called anywhere, at any time, including from interrupt context.
         */

        if (!ptr && !allowNULL)
            return false;

        return true;
    }
 
 private:
    static jmp_buf jmpExit;
};


/**
 * Event dispatcher
 *
 * Pending events are tracked with a hierarchy of change bitmaps.
 * Bitmaps should always be set from most specific to least specific.
 * For example, with asset downloading, an individual cube's bit
 * in assetDoneCubes
 */

class Event {
 public:
    static void dispatch();

    enum Type {
        ASSET_DONE = 0,
        ACCEL_CHANGE,
    };
    
    static void setPending(Type t) {
        Sifteo::Atomic::Or(pending, Sifteo::Intrinsic::LZ(t));
    }

    static bool dispatchInProgress;     /// Reentrancy detector
    static uint32_t pending;            /// CLZ map of all pending events
    static uint32_t assetDoneCubes;     /// CLZ map, by cube slot, of asset download completion
    static uint32_t accelChangeCubes;   /// CLZ map, by cube slot, of accelerometer changes

 private:
    /*
     * Vector entry points.
     *
     * XXX: This is an ugly placeholder for something
     *      probably-machine-specific and data-driven to enter the
     *      interpreter quickly and make an asynchronous procedure call.
     */

    static void cubeFound(_SYSCubeID cid) {
        if (_SYS_vectors.cubeFound)
            _SYS_vectors.cubeFound(cid);
    }

    static void cubeLost(_SYSCubeID cid) {
        if (_SYS_vectors.cubeLost)
            _SYS_vectors.cubeLost(cid);
    }

    static void assetDone(_SYSCubeID cid) {
        if (_SYS_vectors.assetDone)
            _SYS_vectors.assetDone(cid);
    }

    static void accelChange(_SYSCubeID cid) {
        if (_SYS_vectors.accelChange)
            _SYS_vectors.accelChange(cid);
    }
};


#endif
