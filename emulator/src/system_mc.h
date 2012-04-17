/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SYSTEM_MC_H
#define _SYSTEM_MC_H

#include "tinythread.h"

class System;


class SystemMC {
 public:
    bool init(System *sys);
    void exit();

    void start();
    void stop();

 private: 
    void startThread();
    void stopThread();

    static void threadFn(void *param);

    System *sys;
    tthread::thread *mThread;
    bool mThreadRunning;
};

#endif
