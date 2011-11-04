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

namespace EventBits {
	enum ID {
	    // cube event bits first (will be used in pointer maths)
	    CUBEFOUND,
	    CUBELOST,
	    ASSETDONE,
	    ACCELCHANGE,
	    TOUCH,
	    TILT,
	    SHAKE,
	    LAST_CUBE_EVENT = SHAKE,
	    
	    // other event bits
		NEIGHBOR,
		
		// Must be last
	    COUNT      
	};
}

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
         * XXX: We should probably split this into two variants for
         *      two different kinds of user pointers: RAM pointers
         *      (read/write) and virtual flash addresses (read-only).
         *      The variant for virtual flash addresses will also need
         *      to map those addresses (via our cache) into local SRAM
         *      temporarily. Yes, it's yet another software virtual
         *      memory subsystem. VMware deja vu.
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
    
    static void setPending(EventBits::ID t, _SYSCubeID id) {
        Sifteo::Atomic::SetLZ(pending, t);
        Sifteo::Atomic::SetLZ(eventCubes[t], id);
    }

    static bool dispatchInProgress;     /// Reentrancy detector
    static uint32_t pending;            /// CLZ map of all pending events

    /// Each event type has a map by cube slot
    static uint32_t eventCubes[EventBits::COUNT];     
    
 private:
    /*
     * Vector entry points.
     *
     * XXX: This is an ugly placeholder for something
     *      probably-machine-specific and data-driven to enter the
     *      interpreter quickly and make an asynchronous procedure call.
     */
    static void callCubeEvent(EventBits::ID event, _SYSCubeID cid) {
        STATIC_ASSERT( &(_SYS_vectors.cubeEvents.found)+EventBits::CUBEFOUND == &(_SYS_vectors.cubeEvents.found) );
        STATIC_ASSERT( &(_SYS_vectors.cubeEvents.found)+EventBits::CUBELOST == &(_SYS_vectors.cubeEvents.lost) );
        STATIC_ASSERT( &(_SYS_vectors.cubeEvents.found)+EventBits::ASSETDONE == &(_SYS_vectors.cubeEvents.assetDone) );
        STATIC_ASSERT( &(_SYS_vectors.cubeEvents.found)+EventBits::ACCELCHANGE == &(_SYS_vectors.cubeEvents.accelChange) );
        STATIC_ASSERT( &(_SYS_vectors.cubeEvents.found)+EventBits::TOUCH == &(_SYS_vectors.cubeEvents.touch) );
        STATIC_ASSERT( &(_SYS_vectors.cubeEvents.found)+EventBits::TILT == &(_SYS_vectors.cubeEvents.tilt) );
        STATIC_ASSERT( &(_SYS_vectors.cubeEvents.found)+EventBits::SHAKE == &(_SYS_vectors.cubeEvents.shake) );
        ASSERT(cid < _SYS_NUM_CUBE_SLOTS);
        _SYSCubeEvent* eventArray = (_SYSCubeEvent*) &(_SYS_vectors.cubeEvents);
        if (eventArray[event])
             eventArray[event](cid);
    }
};


#endif
