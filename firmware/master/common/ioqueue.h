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

#ifndef IOQUEUE_H
#define IOQUEUE_H

#include "macros.h"
#include "bits.h"
#include "svmmemory.h"
#include <stdint.h>

/**
 * IoQueue is a system abstraction that provides access (typically)
 * to a _SYSBluetoothQueue or _SYSUsbQueue that resides in userspace.
 * The interface is similar to BluetoothQueue/UsbQueue from the SDK, but this class
 * is written with user and system memory protection in mind.
 */

template <class P, class Q>
class IoQueue {
public:

    bool attach(SvmMemory::VirtAddr va) {
        SvmMemory::PhysAddr pa;
        _SYSIoQueueHeader *header;

        // Unset
        queue = NULL;

        if (!va) {
            // Detach successfully
            return true;
        }

        if (!SvmMemory::mapRAM(va, sizeof *header, pa)) {
            // Not even enough room for a valid header
            return false;
        }

        // We read 'last' exactly once, prior to mapping memory.
        header = reinterpret_cast<_SYSIoQueueHeader*>(pa);
        last = header->last;
        unsigned size = last + 1;

        if (!SvmMemory::mapRAM(va, sizeof *header + sizeof(P) * size, pa)) {
            // Not enough room for all packets claimed to be part of this queue
            return false;
        }

        queue = reinterpret_cast<Q*>(pa);
        return true;
    }

    // Zero-copy read
    P *peek() {
        unsigned size = last + 1;
        unsigned head = queue->header.head % size;
        index = head;
        return &queue->packets[head];
    }

    void pop() {
        unsigned head = index + 1;
        if (head > last)
            head = 0;
        queue->header.head = head;
    }


    // Zero-copy write
    P *reserve() {
        unsigned size = last + 1;
        unsigned tail = queue->header.tail % size;
        index = tail;
        return &queue->packets[tail];
    }

    void commit() {
        unsigned tail = index + 1;
        if (tail > last)
            tail = 0;
        queue->header.tail = tail;
    }



    ALWAYS_INLINE bool hasQueue() const {
        return queue != 0;
    }

    ALWAYS_INLINE bool full() const {
        unsigned size = last + 1;
        return (queue->header.tail + 1) % size == queue->header.head;
    }

    ALWAYS_INLINE bool empty() const {
        return queue->header.tail == queue->header.head;
    }

private:
    Q *queue;           // Pointer to mapped user RAM.
    uint8_t last;       // Trusted value of 'last', read once.
    uint8_t index;      // Trusted state kept between peek/pop, reserve/commit
};

#endif // IOQUEUE_H
