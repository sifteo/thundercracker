/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef TASKS_H
#define TASKS_H

#include <stdint.h>

class Tasks
{
public:
    enum TaskID {
        UsbIN,
        UsbOUT,
        AudioPull,
        Debugger,
        AssetLoader,
    };

    static void init();
    static void work();
    static void setPending(TaskID id, void *p = 0, bool runAlways = false);

private:
    typedef void (*TaskCallback)(void *);

    static uint32_t pendingMask;
    static uint32_t alwaysMask;
    struct Task {
        TaskCallback callback;
        void *param;
    };

    static Task TaskList[];

    static void doJobs(uint32_t &mask);
};

#endif // TASKS_H
