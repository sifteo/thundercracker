/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef TASKS_H
#define TASKS_H

#include "macros.h"
#include "machine.h"

#ifndef SIFTEO_SIMULATOR
#include "board.h"
#endif

/*
 * Tasks are a simple form of cooperative multitasking, which operates
 * somewhat like an interrupt controller. Each task has a pending flag,
 * and the task handler itself is responsible for determining what
 * work within its subsystem needs doing.
 *
 * Pending flags can be set or cleared manually, from an ISR or not,
 * at any time. They are auto-cleared before the handler is invoked,
 * so setPending() is one-shot and clearPending() can cancel an earlier
 * setPending() operation.
 *
 * Note that normally atomic operation primitives are required. For example,
 * an ISR may try to set a task pending while the main thread is clearing
 * a different task, leading to the 'set' being lost.
 *
 * Why cooperative multitasking, when we already have hardware interrupts?
 * We use tasks to serialize access to flash memory. All of these tasks
 * are interleaved with user-mode code execution, allowing us to share the
 * flash bus with user code.
 */

class Tasks
{
public:
    // Order defines task priority. Higher priorities come first.
    enum TaskID {
        PowerManager,
        UsbOUT,
        AudioPull,
        FaultLogger,
        Debugger,
        AssetLoader,
        Pause,
        CubeConnector,
        Heartbeat,
        Profiler,
        TestJig
    };

    static void init() {
        pendingMask = 0;
        watchdogCounter = 0;
    }

    /*
     * Run pending tasks until no tasks are pending. Returns 'true' if we did
     * any work, or 'false' of no tasks were pending when we checked.
     *
     * Nested task invocation is allowed. The supplied mask can be used
     * to exclude specific tasks.
     */
    static bool work(uint32_t exclude=0);

    /// Block an idle caller. Runs pending tasks OR waits for a hardware event
    static void idle(uint32_t exclude=0);

    /*
     * Heartbeat ISR handler, called at HEARTBEAT_HZ by a hardware timer.
     * This triggers the Heartbeat task, and acts as a watchdog for Tasks::work().
     *
     * This timer MUST be long enough, worst case, for one 64kB flash block erasure.
     * Typical erase time is 1/2 second, but the data sheet specifies a max of 1 second.
     * To be on the safe side, our timeout is currently 3 seconds.
     */
    static void heartbeatISR();
    static const unsigned HEARTBEAT_HZ = 10;
    static const unsigned WATCHDOG_DURATION = HEARTBEAT_HZ * 3;

    /// One-shot, execute a task once at the next opportunity
    static ALWAYS_INLINE void trigger(TaskID id) {
        Atomic::SetLZ(pendingMask, id);
    }

    // Is a task pending?
    static ALWAYS_INLINE bool isPending(TaskID id) {
        return !!(Intrinsic::LZ(id) & pendingMask);
    }

    /*
     * Cancel a trigger() before the task has run.
     *
     * Cancellation requires care! It is always guaranteed to cancel the task
     * if run from the main thread, either from the same or a different task handler.
     * It is NOT guaranteed to cancel the task if run from an ISR, however.
     * Since it wouldn't be possible to cancel the task during its execution
     * either, this does not weaken any existing atomicity guarantees.
     */
    static void cancel(TaskID id) {
        uint32_t mask = ~Intrinsic::LZ(id);
        Atomic::And(pendingMask, mask);
        iterationMask &= mask;
    }

    /*
     * Nominally this watchdog is only reset during work(), but rarely there will
     * be an event that actually necessitates such a long delay; such as erasing
     * flash memory. So in those rare cases, the watchdog may be reset from other
     * modules.
     */
    static ALWAYS_INLINE void resetWatchdog() {
        watchdogCounter = 0;
    }

    /*
     * Block until the next hardware event occurs.
     * (Emulated in siftulator, one instruction in hardware)
     */
#ifdef SIFTEO_SIMULATOR
    static void waitForInterrupt();
#else
    static ALWAYS_INLINE void waitForInterrupt() {
        #ifndef REV2_GDB_REWORK
        __asm__ __volatile__ ("wfi");
        #endif
    }
#endif

private:

    static uint32_t pendingMask;
    static uint32_t iterationMask;
    static uint32_t watchdogCounter;

    static void heartbeatTask();
    static ALWAYS_INLINE void taskInvoke(unsigned id);
};

#endif // TASKS_H
