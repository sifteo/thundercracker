/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
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
