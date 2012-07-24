/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_RUNTIME_H
#define _SIFTEO_RUNTIME_H

#include <sifteo/abi.h>
#include "machine.h"
#include "svm.h"
#include "svmruntime.h"
#include "bits.h"
using namespace Svm;


/**
 * Event dispatcher
 *
 * Pending events are tracked with a hierarchy of change bitmaps.
 * Bitmaps should always be set from most specific to least specific.
 * For example, with asset downloading, an individual cube's bit
 * in assetDoneCubes.
 *
 * Since the change bits also implicitly prioritize events,
 * and we want to be able to rearrange the priority of events
 * without breaking ABI, we introduce one layer of indirection. This
 * class defines its own Priority IDs (PID) which are used in these
 * change bitmaps.
 *
 * Events with lower priority ID numbers are higher priority, and
 * vice versa.
 */

class Event {
 public:

    // This must stay in sync with pidToVector[] in event.cpp!
    enum PriorityID {

        // Audio events (latency sensitive)
        PID_BASE_TRACKER,

        // High priority: Cube disconnect/connect events
        PID_CONNECTION,

        // Screen refresh. Must be lower priority than connection
        PID_CUBE_REFRESH,

        // Low-bandwidth events
        PID_NEIGHBORS,
        PID_CUBE_TOUCH,
        PID_CUBE_ASSETDONE,
        PID_CUBE_TILT,
        PID_CUBE_SHAKE,

        // High-bandwidth / low-priority events
        PID_CUBE_BATTERY,
        PID_CUBE_ACCELCHANGE,

        // Must be last
        NUM_PIDS
    };

    // Only called by SvmRuntime. Do not call directly from syscalls!
    static void dispatch();

    static void clearVectors();

    static void setBasePending(PriorityID pid, uint32_t param);
    static void setCubePending(PriorityID pid, _SYSCubeID cid);

    static void setVector(_SYSVectorID vid, void *handler, void *context);
    static void *getVectorHandler(_SYSVectorID vid);
    static void *getVectorContext(_SYSVectorID vid);

    static bool callNeighborEvent(_SYSVectorID vid, _SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1);

 private:
    struct VectorInfo {
        reg_t handler;
        reg_t context;
    };

    union Params {
        _SYSCubeIDVector cubesPending;      /// CLZ map of pending cubes
        uint32_t generic;
    };

    static bool callBaseEvent(_SYSVectorID vid, uint32_t param);
    static bool callCubeEvent(_SYSVectorID vid, _SYSCubeID cid);

    static bool dispatchCubePID(PriorityID pid, _SYSCubeID cid);

    static VectorInfo vectors[_SYS_NUM_VECTORS];
    static Params params[NUM_PIDS];
    static BitVector<NUM_PIDS> pending;
};


#endif
