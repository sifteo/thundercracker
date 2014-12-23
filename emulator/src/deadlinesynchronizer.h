/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Micah Elizabeth Scott <micah@misc.name>
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

#ifndef _DEADLINE_SYNCHRONIZER_H
#define _DEADLINE_SYNCHRONIZER_H

#include "tinythread.h"
#include "macros.h"
#include "vtime.h"


/**
 * Provide a way for a VirtualTime-based thread to rally at specific
 * deadlines set by some other external thread. During one of these
 * synchronization events, both threads run synchronously. As soon as the
 * event ends, both threads may resume running. The VirtualTime-based thread
 * can run up until the next deadline.
 */
class DeadlineSynchronizer {
public:
    
    /**
     * Initialize the simulation thread, and bind this synchronizer to a clock.
     */
    void init(const VirtualTime *vtime, bool *tickRunFlag)
    {
        mThreadWaiting = false;
        mInEvent = false;
        mTickRunFlag = tickRunFlag;

        /*
         * Note 1: Must reset to 0 so that we don't run at all until the
         *         first deadline has been set.
         *
         * Note 2: We must initialize atomically to the above state, without
         *         the normal reset state of "infinitely far in the future".
         *         This init() could happen asynchronously with respect to the
         *         MC thread, so we could be init'ing while the MC thread is
         *         waiting in beginEvent(). So, we take advantage of the fact
         *         that we are always allowed to wake up sooner than called
         *         for, just never later, and we use initTo() to to directly
         *         to the distant past.
         *
         * Yes, a very long-winded explanation for one boring line of code...
         */
        mDeadline.initTo(vtime, 0);
    }

    /**
     * Wake up waiters on this DeadlineSynchronizer.
     * From the outside, this has no effect other than to give up
     * if the given 'runFlag' value becomes false.
     */
    void wake()
    {
        DEBUG_LOG(("SYNC: wake\n"));
        mCond.notify_all();
    }

    /**
     * Begin an event that's synchronized with thread execution.
     * Sets 'deadline' as the next halt point, and waits for the thread
     * to halt. Note that the cubes may halt prior to the stated
     * deadline, in the event that they were already waiting when
     * beginEvent() was called.
     */
    void beginEvent(uint64_t deadline, bool &runFlag)
    {
        DEBUG_LOG(("SYNC: +beginEvent(%"PRIu64") run=%d\n", deadline, runFlag));

        ASSERT(!mInEvent);
        mMutex.lock();
        mDeadline.resetTo(deadline);
        mInEvent = true;

        while (!mThreadWaiting && runFlag) {
            wake();
            mCond.wait(mMutex);
        }

        DEBUG_LOG(("SYNC: -beginEvent(%"PRIu64") run=%d\n", deadline, runFlag));
    }

    /**
     * Begin an event, at exactly the specified deadline. This will
     * begin multiple sync events if necessary, in order to roll the
     * clock forward to 'deadline'. The previous deadline must not have
     * been greater than the exact deadline requested here.
     */
    void beginEventAt(uint64_t deadline, bool &runFlag)
    {
        beginEvent(deadline, runFlag);
        if (!runFlag)
            return;

        // beginCubeEvent may have already been waiting when we called, and it could
        // be earlier than the deadline we asked for. Now that we're sync'ed, run
        // up until the proper deadline if necessary.

        ASSERT(deadline >= mDeadline.clock());
        while (deadline > mDeadline.clock()) {
            endEvent(deadline);
            beginEvent(deadline, runFlag);
            if (!runFlag)
                return;
        }

        ASSERT(deadline == mDeadline.clock());
    }

    /**
     * End an event, resume thread execution.
     * Let it get as far as 'nextDeadline' without stopping.
     */
    void endEvent(uint64_t nextDeadline)
    {
        DEBUG_LOG(("SYNC: +endEvent(%"PRIu64")\n", nextDeadline));

        // Can almost ASSERT(mThreadWaiting) here too, but that breaks down
        // when one thread is exiting, and there's no good way to detect that here.

        ASSERT(mInEvent);
        mDeadline.resetTo(nextDeadline);

        mThreadWaiting = false;
        mCond.notify_all();
        mInEvent = false;
        mMutex.unlock();

        DEBUG_LOG(("SYNC: -endEvent(%"PRIu64")\n", nextDeadline));
    }

    /**
     * Tick handler for the simulation thread
     */
    ALWAYS_INLINE void tick()
    {
        if (mDeadline.hasPassed())
            deadlineWork();
    }
    
    /**
     * Remaining ticks before the deadline
     */
    ALWAYS_INLINE uint64_t remaining()
    {
        return mDeadline.remaining();
    }
    
private:
    NEVER_INLINE void deadlineWork()
    {
        DEBUG_LOG(("SYNC: +deadlineWork (run=%d)\n", *mTickRunFlag));

        tthread::lock_guard<tthread::mutex> guard(mMutex);
        mThreadWaiting = true;
        while (mThreadWaiting && *mTickRunFlag) {
            wake();
            mCond.wait(mMutex);
        }

        DEBUG_LOG(("SYNC: -deadlineWork (run=%d)\n", *mTickRunFlag));
    }

    TickDeadline mDeadline;
    tthread::mutex mMutex;
    tthread::condition_variable mCond;

    // Run flag for the tick thread.
    bool *mTickRunFlag;

    // Set to 'true' by tick thread, 'false' by external thread
    bool mThreadWaiting;
    
    // Between begin and end? For ASSERTs only.
    bool mInEvent;
};


#endif
