/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SYSTEM_CUBES_H
#define _SYSTEM_CUBES_H

#include "tinythread.h"
#include "macros.h"
#include "vtime.h"

class System;


class SystemCubes {
 public:
    SystemCubes() : mEventAtDeadline(false), mEventDone(false) {}

    bool init(System *sys);
    void exit();

    void start();
    void stop();

    void setNumCubes(unsigned n);
    void resetCube(unsigned id);

    // Begin an event that's synchronized with cube execution. Halts the cube thread at 'deadline'.
    void beginEvent(uint64_t deadline) {
        tthread::lock_guard<tthread::mutex> guard(mEventMutex);
        mEventDeadline.resetTo(deadline);
        while (!mEventAtDeadline)
            mEventAtDeadlineCond.wait(mEventMutex);
        mEventAtDeadline = false;
    }

    // End an event, resume cube execution. Let it get as far as 'nextDeadline' without stopping.
    void endEvent(uint64_t nextDeadline) {
        tthread::lock_guard<tthread::mutex> guard(mEventMutex);
        mEventDeadline.resetTo(nextDeadline);
        mEventDone = true;
        mEventDoneCond.notify_all();
    }

 private: 
    void startThread();
    void stopThread();

    static void threadFn(void *param);
    bool initCube(unsigned id, bool wakeFromSleep=false);
    void exitCube(unsigned id);

    ALWAYS_INLINE void tick(unsigned count=1);
    NEVER_INLINE void tickLoopDebug();
    NEVER_INLINE void tickLoopGeneral();
    NEVER_INLINE void tickLoopFastSBT();

    System *sys;
    tthread::thread *mThread;
    bool mThreadRunning;

    TickDeadline mEventDeadline;
    tthread::mutex mEventMutex;
    bool mEventAtDeadline, mEventDone;
    tthread::condition_variable mEventAtDeadlineCond;
    tthread::condition_variable mEventDoneCond;
};

#endif
