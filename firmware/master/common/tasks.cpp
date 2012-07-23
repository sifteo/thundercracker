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
#include "powermanager.h"
#endif

uint32_t Tasks::pendingMask;

/*
 * Table of runnable tasks.
 */

static ALWAYS_INLINE void taskInvoke(unsigned id)
{
    switch (id) {

    #ifndef SIFTEO_SIMULATOR
        case Tasks::PowerManager:   return PowerManager::vbusDebounce();
        case Tasks::UsbOUT:         return UsbDevice::handleOUTData();
    #endif

    #ifndef BOOTLOADER
        case Tasks::AudioPull:      return AudioMixer::pullAudio();
        case Tasks::Debugger:       return SvmDebugger::messageLoop();
        case Tasks::AssetLoader:    return CubeSlots::assetLoaderTask();
        case Tasks::HomeButton:     return HomeButton::task();
    #endif

    #if !defined(SIFTEO_SIMULATOR) && !defined(BOOTLOADER)
        case Tasks::Profiler:       return SampleProfiler::task();
    #endif

    }
}

void Tasks::init()
{
    pendingMask = 0;
}

/*
 * Service any tasks that are pending since our last round of work.
 * Should be called in main thread context, as callbacks might typically
 * take some time - it's the whole reason Tasks exists, after all.
 */
void Tasks::work()
{
    uint32_t mask = pendingMask;

    while (mask) {
        // Must clear the bit before invoking the callback
        unsigned idx = Intrinsic::CLZ(mask);
        mask ^= Intrinsic::LZ(idx);
        taskInvoke(idx);
    }
}

/*
 * Run pending tasks, OR if no tasks are pending, wait for interrupts.
 * This is the correct way to block the main thread of execution while
 * waiting for a condition.
 */
void Tasks::idle()
{
    /*
     * XXX: This should really exit before waitForInterrupt(), in the
     *      event that we're actually making forward progress in work().
     *      Right now there isn't a good way of detecting this, though.
     *
     *      I'd like to call waitForInterrupt() if and only if there
     *      were no pending tasks, but this currently doesn't work because
     *      of tasks like AudioPull which are almost always runnable.
     *
     *      So, until we find a clean solution to that problem, idle() is
     *      equivalent to work() followed by waitForInterrupt(). But we're
     *      still consolidating our idling logic as much as possible so that
     *      this strategy can be changed easily later.
     */

    work();
    waitForInterrupt();
}
