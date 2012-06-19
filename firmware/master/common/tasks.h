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
        UsbOUT,
        AudioPull,
        Debugger,
        AssetLoader,
        HomeButton,
        Profiler
    };

    static void init();
    static void work();
    static void idle();

    /*
     * Tasks that have been set pending get called once each time
     * Tasks::work() is run.
     *
     * Call clearPending() to unregister your task. You can pend it again
     * at any time.
     */
    static void setPending(TaskID id, void *p = 0);
    static void clearPending(TaskID id);

private:
    typedef void (*TaskCallback)(void *);

    static uint32_t pendingMask;
    struct Task {
        TaskCallback callback;
        void *param;
    };

    static Task TaskList[];
};

#endif // TASKS_H
