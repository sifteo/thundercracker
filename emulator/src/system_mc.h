/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SYSTEM_MC_H
#define _SYSTEM_MC_H

#include <setjmp.h>
#include "tinythread.h"

class System;
class Radio;
struct SysTime;
struct RadioAddress;
class CubeSlot;

namespace Cube {
    class Hardware;
}


class SystemMC {
 public:
    bool init(System *sys);
    void exit();

    void start();
    void stop();

    static Cube::Hardware *getCubeForSlot(CubeSlot *slot);
    static void checkQuiescentVRAM(CubeSlot *slot);

    static bool installELF(const char *name);

    static System *getSystem() {
        return instance->sys;
    }

    /**
     * Cause some time to pass in the MC simulation, and service any
     * asynchronous events that occurred during this elapsed time.
     *
     * Must only be called from the MC thread.
     */
    static void elapseTicks(unsigned n);

 private: 
    static void threadFn(void *);
    void doRadioPacket();

    Cube::Hardware *getCubeForAddress(const RadioAddress *addr);

    friend class Radio;
    friend struct SysTime;

    static SystemMC *instance;
    uint64_t ticks;
    uint64_t radioPacketDeadline;

    System *sys;
    tthread::thread *mThread;
    bool mThreadRunning;
    jmp_buf mThreadExitJmp;
};

#endif
