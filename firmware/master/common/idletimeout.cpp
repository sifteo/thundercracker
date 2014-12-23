/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "idletimeout.h"
#include "machine.h"
#include "tasks.h"
#include "pause.h"
#include "audiomixer.h"

#include "ui_coordinator.h"
#include "ui_shutdown.h"
#include "svmclock.h"

#ifndef SIFTEO_SIMULATOR
#   include "powermanager.h"
#endif

unsigned IdleTimeout::countdown = IdleTimeout::IDLE_TIMEOUT_SYSTICKS;

void IdleTimeout::heartbeat()
{
    ASSERT(countdown > 0);

    /*
     * TODO: if we've already shut down, but we're still alive since we're plugged
     * in via USB, don't bother counting down to yet another shutdown event.
     */

    if (--countdown == 0) {

        reset();    // in case we come back, without actually powering down

        // ensure any playing audio is fully flushed from the audio device
        AudioMixer::instance.fadeOut();
        const uint32_t excludedFlushTasks =
                Intrinsic::LZ(Tasks::Pause)         |
                Intrinsic::LZ(Tasks::Heartbeat)     |
                Intrinsic::LZ(Tasks::FaultLogger);
        while (!AudioMixer::instance.outputBufferIsSilent()) {
            Tasks::work(excludedFlushTasks);
        }

        if (!SvmClock::isPaused())
            SvmClock::pause();

        /*
         * It's important that we *do not* run in the Pause task, so that we
         * can still shutdown on idle timeout even when the system has
         * already been paused.
         */

        const uint32_t excludedTasks =
            Intrinsic::LZ(Tasks::AudioPull)     |
            Intrinsic::LZ(Tasks::Pause)         |
            Intrinsic::LZ(Tasks::Heartbeat)     |
            Intrinsic::LZ(Tasks::FaultLogger);

        UICoordinator uic(excludedTasks);
        UIShutdown uiShutdown(uic);
        uiShutdown.mainLoop();
    }
}
