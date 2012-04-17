/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include "system_mc.h"


bool SystemMC::init(System *sys)
{
    this->sys = sys;
    return true;
}

void SystemMC::startThread()
{
    mThreadRunning = true;
    __asm__ __volatile__ ("" : : : "memory");
    mThread = new tthread::thread(threadFn, this);
}

void SystemMC::stopThread()
{
    mThreadRunning = false;
    __asm__ __volatile__ ("" : : : "memory");
    mThread->join();
    delete mThread;
    mThread = 0;
}

void SystemMC::start()
{
    startThread();
}

void SystemMC::stop()
{
    stopThread();
}

void SystemMC::exit()
{
}

void SystemMC::threadFn(void *param)
{
#if 0 // xxx
    SystemMC *self = (SystemMC *) param;
    System *sys = self->sys;

    while (self->mThreadRunning) {
    }
#endif
}
