/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

#include <stdint.h>
#include "macros.h"


/**
 * Template for a statically-sized ring buffer.
 *
 * NOT suitable for use across the userspace/system boundary, as we
 * treat the head/tail pointers as trusted values.
 */
template <unsigned tSize, typename tItemType = uint8_t, typename tIndexType = uint16_t>
class RingBuffer
{
    tIndexType mHead;      /// Index of the next item to read
    tIndexType mTail;      /// Index of the next empty slot to write into
    tItemType mBuf[tSize];

public:
    RingBuffer()
    {}

    void init() {
        // must be power of 2
        STATIC_ASSERT((tSize & (tSize - 1)) == 0);
        mHead = mTail = 0;
    }

    inline unsigned capacity() const {
        return tSize - 1;
    }

    void enqueue(tItemType c)
    {
        ASSERT(!full());
        unsigned tail = mTail;
        mBuf[tail] = c;
        mTail = capacity() & (tail + 1);
    }

    tItemType dequeue()
    {
        ASSERT(!empty());
        unsigned head = mHead;
        tItemType c = mBuf[head];
        mHead = capacity() & (head + 1);
        return c;
    }

    /// Copy from 'src' to 'this' until the source is empty or destination full.
    template <typename T>
    void pull(T &src)
    {
        unsigned rCount = src.readAvailable();
        unsigned wCount = writeAvailable();
        unsigned count = MIN(rCount, wCount);
        while (count--) {
            enqueue(src.dequeue());
        }
    }

    inline bool full() const {
        return (capacity() & (mTail + 1)) == mHead;
    }

    inline bool empty() const {
        return mHead == mTail;
    }

    unsigned readAvailable() const
    {
        unsigned head = mHead;
        unsigned tail = mTail;
        return (tail - head) & capacity();
    }

    unsigned writeAvailable() const {
        return capacity() - readAvailable();
    }
};

#endif // RING_BUFFER_H_
