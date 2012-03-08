/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_RUNTIME_H
#define _SIFTEO_RUNTIME_H

#include <sifteo/abi.h>
#include <sifteo/machine.h>
#include "svm.h"
#include "svmruntime.h"
using namespace Svm;


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

    // Only called by SvmRuntime. Do not call directly from syscalls!
    static void dispatch();
    
    static void setPending(_SYSVectorID vid, _SYSCubeID cid) {
        ASSERT(vid < _SYS_NUM_VECTORS);
        ASSERT(cid < _SYS_NUM_CUBE_SLOTS);
        Sifteo::Atomic::SetLZ(pending, vid);
        Sifteo::Atomic::SetLZ(vectors[vid].cubesPending, cid);
    }
    
    static void setVector(_SYSVectorID vid, void *handler, void *context) {
        ASSERT(vid < _SYS_NUM_VECTORS);
        vectors[vid].handler = reinterpret_cast<reg_t>(handler);
        vectors[vid].context = reinterpret_cast<reg_t>(context);
    }
    
    static void *getVectorHandler(_SYSVectorID vid) {
        ASSERT(vid < _SYS_NUM_VECTORS);
        return reinterpret_cast<void*>(vectors[vid].handler);
    }

    static void *getVectorContext(_SYSVectorID vid) {
        ASSERT(vid < _SYS_NUM_VECTORS);
        return reinterpret_cast<void*>(vectors[vid].context);
    }
	
    static inline bool callCubeEvent(_SYSVectorID vid, _SYSCubeID cid) {
        ASSERT(vid < _SYS_NUM_VECTORS);
        ASSERT(cid < _SYS_NUM_CUBE_SLOTS);
        ASSERT(Sifteo::Intrinsic::LZ(vid) & _SYS_CUBE_EVENTS);
        VectorInfo &vi = vectors[vid];

        if (vi.handler) {
            SvmRuntime::sendEvent(vi.handler, vi.context, cid);
            return true;
        }

        return false;
    }

    static inline bool callNeighborEvent(_SYSVectorID vid,
        _SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1) {
        ASSERT(vid < _SYS_NUM_VECTORS);
        ASSERT(Sifteo::Intrinsic::LZ(vid) & _SYS_NEIGHBOR_EVENTS);
        VectorInfo &vi = vectors[vid];

        if (vi.handler) {
            SvmRuntime::sendEvent(vi.handler, vi.context, c0, s0, c1, s1);
            return true;
        }
        
        return false;
    }

 private:
     
    struct VectorInfo {
        reg_t handler;
        reg_t context;
        union {                                 /// Type-specific state:
            _SYSCubeIDVector cubesPending;      /// CLZ map of pending cubes
        };
    };

    static VectorInfo vectors[_SYS_NUM_VECTORS];
    static uint32_t pending;            /// CLZ map of all pending events
};


#endif
