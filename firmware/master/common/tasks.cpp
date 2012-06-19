/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "tasks.h"
#include "audiomixer.h"
#include "svmdebugger.h"
#include "cubeslots.h"
#include "homebutton.h"

#ifndef SIFTEO_SIMULATOR
#include "usb/usbdevice.h"
#include "sampleprofiler.h"
#endif

uint32_t Tasks::pendingMask;

// XXX: managing the list of tasks for different configurations
// is starting to get a little unwieldy...
Tasks::Task Tasks::TaskList[] = {
    #ifdef SIFTEO_SIMULATOR
    { 0 },
    #else
    { UsbDevice::handleOUTData, 0},
    #endif

    #ifndef BOOTLOADER
    { AudioMixer::pullAudio, 0},
    { SvmDebugger::messageLoop, 0},
    { CubeSlots::assetLoaderTask, 0 },
    { HomeButton::task, 0 },
    #endif

    #ifdef SIFTEO_SIMULATOR
    { 0 },
    #else
    { SampleProfiler::task, 0 },
    #endif
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
    ASSERT((unsigned)id < arraysize(TaskList));
    Task &task = TaskList[id];
    ASSERT(task.callback != NULL);
    task.param = p;

    Atomic::SetLZ(pendingMask, id);
}

void Tasks::clearPending(TaskID id)
{
    Atomic::ClearLZ(pendingMask, id);
}

/*
    Service any tasks that are pending since our last round of work.
    Should be called in main thread context, as callbacks might typically
    take some time - it's the whole reason Tasks exists, after all.
*/
void Tasks::work()
{
    uint32_t mask = pendingMask;
    while (mask) {
        unsigned idx = Intrinsic::CLZ(mask);
        // clear before calling back since callback might take a while and
        // the flag might get set again in the meantime
        Atomic::ClearLZ(mask, idx);

        Task &task = TaskList[idx];
        task.callback(task.param);
    }
}

#ifndef SIFTEO_SIMULATOR
void Tasks::idle()
{
    __asm__ __volatile__ ("wfi");
}
#endif
