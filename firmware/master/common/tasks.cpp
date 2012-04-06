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

Tasks::Task Tasks::TaskList[] = {
    #ifdef SIFTEO_SIMULATOR
    { 0 },
    { 0 },
    #else
    { UsbDevice::handleINData, 0 },
    { UsbDevice::handleOUTData, 0 },
    #endif
    { AudioMixer::handleAudioOutEmpty, 0 },
    { SvmDebugger::messageLoop, 0 },
    { CubeSlots::assetLoaderTask, 0 },
};

void Tasks::init()
{
    pendingMask = 0;
}

/*
    Pend a given task handler to be run the next time we have time.
*/
void Tasks::setPending(TaskID id, void* p)
{
    ASSERT((unsigned)id < (unsigned)arraysize(TaskList));
    ASSERT(TaskList[id].callback != NULL);
    TaskList[id].param = p;
    Atomic::SetLZ(pendingMask, id);
}

/*
    Service any tasks that are pending since our last round of work.
    Should be called in main thread context, as callbacks might typically
    take some time - it's the whole reason Tasks exists, after all.
*/
void Tasks::work()
{
    // Always try to fetch audio data
    AudioMixer::instance.fetchData();
    
    while (pendingMask) {
        unsigned idx = Intrinsic::CLZ(pendingMask);
        Task &task = TaskList[idx];

        // clear before calling back since callback might take a while and
        // the flag might get set again in the meantime
        Atomic::ClearLZ(pendingMask, idx);

        task.callback(task.param);
    }
}
