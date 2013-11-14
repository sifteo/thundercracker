/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "tasks.h"
#include "svmruntime.h"
#include "audiomixer.h"
#include "svmdebugger.h"
#include "assetloader.h"
#include "pause.h"
#include "cubeconnector.h"
#include "radio.h"
#include "shutdown.h"
#include "idletimeout.h"
#include "faultlogger.h"
#include "batterylevel.h"
#include "volume.h"
#include "btprotocol.h"

#ifdef SIFTEO_SIMULATOR
#   include "mc_timing.h"
#   include "system_mc.h"
#   include "system.h"
#   include "batterylevel.h"
#else
#   include "nrf8001/nrf8001.h"
#   include "usb/usbdevice.h"
#   include "sampleprofiler.h"
#   include "powermanager.h"
#   include "factorytest.h"
#   if (BOARD == BOARD_TEST_JIG)
#       include "testjig.h"
#   endif
#endif


/*
 * Table of runnable tasks.
 */

ALWAYS_INLINE void Tasks::taskInvoke(unsigned id)
{
    switch (id) {

    #ifndef SIFTEO_SIMULATOR
        #if BOARD != BOARD_TEST_JIG
        case Tasks::PowerManager:       return PowerManager::vbusDebounce();
        #endif

        case Tasks::UsbOUT:             return UsbDevice::handleOUTData();
        case Tasks::UsbIN:              return USBProtocol::inTask();

        #if (BOARD == BOARD_TEST_JIG && !defined(BOOTLOADER))
        case Tasks::TestJig:            return TestJig::task();
        #endif
    #endif

    #if !defined(BOOTLOADER) && !BOARD_EQUALS(BOARD_TEST_JIG)
        case Tasks::AudioPull:          return AudioMixer::pullAudio();
        case Tasks::Debugger:           return SvmDebugger::messageLoop();
        case Tasks::AssetLoader:        return AssetLoader::task();
        case Tasks::Pause:              return Pause::task();
        case Tasks::CubeConnector:      return CubeConnector::task();
        case Tasks::Heartbeat:          return heartbeatTask();
        case Tasks::FaultLogger:        return FaultLogger::task();
        case Tasks::BluetoothProtocol:  return BTProtocol::task();
    #endif

    #if !defined(SIFTEO_SIMULATOR) && defined(HAVE_NRF8001) && !defined(BOOTLOADER)
        case Tasks::BluetoothDriver:    return NRF8001::instance.task();
    #endif

    #if !defined(SIFTEO_SIMULATOR) && !defined(BOOTLOADER) && (BOARD != BOARD_TEST_JIG)
        case Tasks::Profiler:           return SampleProfiler::task();
        case Tasks::FactoryTest:        return FactoryTest::task();
    #endif

    }
}


/*
 * Table of heartbeat actions
 */
void Tasks::heartbeatTask()
{
    /*
     * If both volume and battery sampling are done via ADC,
     * they need to take turns. Battery sample is kicked off
     * in the Volume completion ISR.
     */
#ifdef USE_ADC_FADER_MEAS
    Volume::beginCapture();
#elif defined(USE_ADC_BATT_MEAS)
    BatteryLevel::beginCapture();
#endif

#if !BOARD_EQUALS(BOARD_TEST_JIG)

    #ifndef DISABLE_IDLETIMEOUT
    IdleTimeout::heartbeat();
    #endif

    Radio::heartbeat();
    AssetLoader::heartbeat();

#endif

#ifdef SIFTEO_SIMULATOR
    BatteryLevel::heartbeat();
#endif
}


/***********************************************************************************
 ***********************************************************************************/

uint32_t Tasks::pendingMask;
uint32_t Tasks::iterationMask;
uint32_t Tasks::watchdogCounter;


bool Tasks::work(uint32_t exclude)
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

    resetWatchdog();

    // Quickest possible early-exit
    uint32_t tasks = pendingMask & ~exclude;
    if (LIKELY(!tasks))
        return false;

    /*
     * Elapse some time in simulation. Besides factoring in as part of our
     * timing model, this ensures that a Tasks::work() loop or Tasks::idle()
     * will always elaps a nonzero amount of time, even if there's a task
     * queued which is always triggered. This prevents us from getting into
     * an infinite loop in such cases.
     */
    #ifdef SIFTEO_SIMULATOR
    SystemMC::elapseTicks(MCTiming::TICKS_PER_TASKS_WORK);
    #endif

    // Clear only the bits we managed to capture above
    Atomic::And(pendingMask, ~tasks);

    /*
     * Merge the tasks captured above with any existing iterationMask.
     * We need to be careful of edge cases introduced by running work()
     * while already in a task handler. Tasks already in iterationMask
     * but not yet run may need to be either kept or moved back
     * to pendingMask, depending on whether they're excluded.
     *
     * Also note that iterationMask doesn't need to be atomic here, since
     * we aren't required to catch changes to it made outside of task handlers.
     */

    tasks |= iterationMask;
    uint32_t pendingExcluded = tasks & exclude;
    if (pendingExcluded) {
        Atomic::Or(pendingMask, pendingExcluded);
        tasks ^= pendingExcluded;
    }

    ASSERT((tasks & exclude) == 0);
    iterationMask = tasks;

    do {
        unsigned idx = Intrinsic::CLZ(tasks);
        taskInvoke(idx);
        tasks = (iterationMask &= ~Intrinsic::LZ(idx));
    } while (tasks);

    return true;
}

void Tasks::idle(uint32_t exclude)
{
    /*
     * Run pending tasks, OR if no tasks are pending, wait for interrupts.
     * This is the correct way to block the main thread of execution while
     * waiting for a condition, as it avoids unnecessary WFIs when the
     * caller is waiting on something which requires Tasks to execute.
     */

    if (!work(exclude))
        waitForInterrupt();
}

void Tasks::heartbeatISR()
{
    #ifndef DISABLE_WATCHDOG

    // Check the watchdog timer
    if (++watchdogCounter >= WATCHDOG_DURATION) {

        /*
         * Help diagnose the hang for internal firmware debugging
         */
        #if !defined(BOOTLOADER) && (BOARD != BOARD_TEST_JIG)

            #if !defined(SIFTEO_SIMULATOR)
            SampleProfiler::reportHang();
            #endif

            /*
             * XXX: It's unlikely that we can recover from this fault currently.
             *      If it's a system hang, we're hosed anyway. If it's a userspace
             *      hang, we can log this message to the UART, but PanicMessenger
             *      isn't going to work correctly inside the systick ISR, and we
             *      can't yet regain control over the CPU from a runaway userspace
             *      loop within one block. If this is a userspace hang, we should be
             *      replacing all cached code pages with fields of BKPT instructions
             *      or something equally heavyhanded.
             */
            SvmRuntime::fault(Svm::F_NOT_RESPONDING);
        #endif
    }
    #endif // DISABLE_WATCHDOG

    // Defer to a Task for everything else
    trigger(Heartbeat);
}
