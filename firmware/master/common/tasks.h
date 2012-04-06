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
    static const uint8_t MAX_TASKS = 32;

    enum TaskID {
        UsbIN,
        UsbOUT,
        AudioPull,
        Debugger,
    };

    static void init();
    static void work();
    static void setPending(TaskID id, void *p = 0, bool runAlways = false);

private:
    typedef void (*TaskCallback)(void *);

    static uint32_t pendingMask;
    struct Task {
        TaskCallback callback;
        void *param;
        bool runAlways;
    };

    static Task TaskList[];
};

#endif // TASKS_H
