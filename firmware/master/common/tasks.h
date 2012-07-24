/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef TASKS_H
#define TASKS_H

#include "macros.h"
#include "machine.h"


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
        Debugger,
        AssetLoader,
        HomeButton,
        CubeConnector,
        Profiler,
    };

    static void init() {
        pendingMask = 0;
    }

    /*
     * Run pending tasks until no tasks are pending. Returns 'true' if we did
     * any work, or 'false' of no tasks were pending when we checked.
     */
    static bool work();

    /// Block an idle caller. Runs pending tasks OR waits for a hardware event
    static void idle();

    /// One-shot, execute a task once at the next opportunity
    static void trigger(TaskID id) {
        Atomic::SetLZ(pendingMask, id);
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
     * Block until the next hardware event occurs.
     * (Emulated in siftulator, one instruction in hardware)
     */
#ifdef SIFTEO_SIMULATOR
    static void waitForInterrupt();
#else
    static ALWAYS_INLINE void waitForInterrupt() {
        __asm__ __volatile__ ("wfi");
    }
#endif

private:
    static uint32_t pendingMask;
    static uint32_t iterationMask;
};

#endif // TASKS_H
