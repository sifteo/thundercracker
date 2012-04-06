/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "tasks.h"
#include "audiomixer.h"
#include "svmdebugger.h"

#ifndef SIFTEO_SIMULATOR
#include "usb/usbdevice.h"
#endif

uint32_t Tasks::pendingMask;

Tasks::Task Tasks::TaskList[] = {
    #ifdef SIFTEO_SIMULATOR
    { 0 },
    { 0 },
    #else
    { UsbDevice::handleINData, 0, false},
    { UsbDevice::handleOUTData, 0, false},
    #endif
    { AudioMixer::pullAudio, 0, false},
    { SvmDebugger::messageLoop, 0, false},
};

void Tasks::init()
{
    pendingMask = 0;
}

/*
    Pend a given task handler to be run the next time we have time.
*/
void Tasks::setPending(TaskID id, void* p, bool runAlways)
{
    ASSERT((unsigned)id < (unsigned)arraysize(TaskList));
    Task &task = TaskList[id];
    ASSERT(task.callback != NULL);
    task.param = p;
    task.runAlways = runAlways;

    Atomic::SetLZ(pendingMask, id);
}

/*
    Service any tasks that are pending since our last round of work.
    Should be called in main thread context, as callbacks might typically
    take some time - it's the whole reason Tasks exists, after all.
*/
void Tasks::work()
{
    for (unsigned i = 0; i < arraysize(TaskList); i++) {
        Task &task = TaskList[i];
        if (task.runAlways == true && task.callback) {
            task.callback(task.param);
        }
    }
    
    while (pendingMask) {
        unsigned idx = Intrinsic::CLZ(pendingMask);
        // clear before calling back since callback might take a while and
        // the flag might get set again in the meantime
        Atomic::ClearLZ(pendingMask, idx);

        Task &task = TaskList[idx];
        if (task.runAlways == false) {
            task.callback(task.param);
        }
    }
}
