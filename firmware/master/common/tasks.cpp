/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "tasks.h"
#include "audiomixer.h"
#include "svmdebugger.h"
#include "cubeslots.h"

#ifndef SIFTEO_SIMULATOR
#include "usb/usbdevice.h"
#endif

uint32_t Tasks::pendingMask;
uint32_t Tasks::alwaysMask;

Tasks::Task Tasks::TaskList[] = {
    #ifdef SIFTEO_SIMULATOR
    { 0 },
    { 0 },
    #else
    { UsbDevice::handleINData, 0},
    { UsbDevice::handleOUTData, 0},
    #endif
    { AudioMixer::pullAudio, 0},
    { SvmDebugger::messageLoop, 0},
    { CubeSlots::assetLoaderTask, 0 },
};

void Tasks::init()
{
    pendingMask = 0;
    alwaysMask = 0;
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
    
    if (runAlways) {
        Atomic::SetLZ(alwaysMask, id);
    } else {
        Atomic::SetLZ(pendingMask, id);
    }
}

/*
    Service any tasks that are pending since our last round of work.
    Should be called in main thread context, as callbacks might typically
    take some time - it's the whole reason Tasks exists, after all.
*/
void Tasks::work()
{
    uint32_t always = alwaysMask;
    doJobs(always);
    doJobs(pendingMask);
}

void Tasks::doJobs(uint32_t &mask) {
    while (mask) {
        unsigned idx = Intrinsic::CLZ(mask);
        // clear before calling back since callback might take a while and
        // the flag might get set again in the meantime
        Atomic::ClearLZ(mask, idx);

        Task &task = TaskList[idx];
        task.callback(task.param);
    }
}
