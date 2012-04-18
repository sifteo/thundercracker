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
class SysTime;
class RadioAddress;
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

 private: 
    static const uint32_t TICK_HZ = 16000000;
    static const uint32_t TICKS_PER_PACKET = 7200;       // 450us, minimum packet period
    static const uint32_t MAX_RETRIES = 150;             // Simulates (hardware * software) retries
    static const uint32_t STARTUP_DELAY = TICK_HZ / 4;   // 1/4 second from cube to MC startup

    bool installELF(const char *name);
    static void threadFn(void *);

    void doRadioPacket();
    void beginPacket();
    void endPacket();

    Cube::Hardware *getCubeForSlot(CubeSlot *slot);
    Cube::Hardware *getCubeForAddress(const RadioAddress *addr);

    friend class Radio;
    friend class SysTime;

    static SystemMC *instance;
    uint64_t ticks;

    System *sys;
    tthread::thread *mThread;
    bool mThreadRunning;
    jmp_buf mThreadExitJmp;
};

#endif
