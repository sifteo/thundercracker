/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2013 Sifteo, Inc.
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

#ifndef BTQUEUE_H
#define BTQUEUE_H

#include "macros.h"
#include "bits.h"
#include "svmmemory.h"
#include <stdint.h>

/**
 * BTQueue is a system abstraction that provides access to a _SYSBluetoothQueue
 * that resides in userspace. The interface is similar to BluetoothQueue from the
 * SDK, but this class is written with user and system memory protection in mind.
 */

class BTQueue {
public:
    
    bool attach(SvmMemory::VirtAddr va);
    unsigned count() const;

    // Zero-copy read
    _SYSBluetoothPacket *peek();
    void pop();

    // Zero-copy write
    _SYSBluetoothPacket *reserve();
    void commit();

    ALWAYS_INLINE bool hasQueue() const
    {
        return queue != 0;
    }

    ALWAYS_INLINE unsigned readAvailable() const
    {
        return count();
    }

    ALWAYS_INLINE unsigned writeAvailable() const
    {
        return last - count();
    }

private:
    _SYSBluetoothQueue *queue;      // Pointer to mapped user RAM.
    uint8_t last;                   // Trusted value of 'last', read once.
    uint8_t index;                  // Trusted state kept between peek/pop, reserve/commit
};

#endif
