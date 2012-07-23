/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "tasks.h"
#include "audiomixer.h"
#include "svmdebugger.h"
#include "cubeslots.h"
#include "homebutton.h"
#include "cubeconnector.h"

#ifndef SIFTEO_SIMULATOR
#include "usb/usbdevice.h"
#include "sampleprofiler.h"
#include "powermanager.h"
#endif

uint32_t Tasks::pendingMask;
uint32_t Tasks::iterationMask;

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
        case Tasks::CubeConnector:  return CubeConnector::task();
    #endif

    #if !defined(SIFTEO_SIMULATOR) && !defined(BOOTLOADER)
        case Tasks::Profiler:       return SampleProfiler::task();
    #endif

    }
}

bool Tasks::work()
{
    /*
     * We sample pendingMask exactly once, and use this to quickly make
     * the determination of whether any work is to be done or not.
     *
     * If we do have some amount of work to do, we atomically transfer
     * all pending flags from pendingMask to iterationMask, where we
     * clear the flags as we invoke each callback.
     *
     * This method is efficient in the common cases, and it is safe
     * even in tricky edge cases like tasks which re-trigger themselves
     * but may be cancelled by a different task. The iterationMask
     * is also cancelled if we cancel a task, to handle the case where
     * the cancelled task was already pulled from pendingMask.
     */

    // Quickest possible early-exit
    uint32_t tasks = pendingMask;
    if (LIKELY(!tasks))
        return false;

    // Clear only the bits we managed to capture above
    Atomic::And(pendingMask, ~tasks);

    // Doesn't need to be atomic; we're not required to catch changes to
    // iterationMask made outside of task handlers (i.e. in ISRs)
    iterationMask = tasks;

    do {
        unsigned idx = Intrinsic::CLZ(tasks);
        taskInvoke(idx);
        tasks = (iterationMask &= ~Intrinsic::LZ(idx));
    } while (tasks);

    return true;
}

void Tasks::idle()
{
    /*
     * Run pending tasks, OR if no tasks are pending, wait for interrupts.
     * This is the correct way to block the main thread of execution while
     * waiting for a condition, as it avoids unnecessary WFIs when the
     * caller is waiting on something which requires Tasks to execute.
     */

    if (!work())
        waitForInterrupt();
}
