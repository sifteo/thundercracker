/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
 */

#include "btqueue.h"

bool BTQueue::attach(SvmMemory::VirtAddr va)
{
    SvmMemory::PhysAddr pa;
    _SYSBluetoothQueueHeader *header;

    // Unset
    queue = 0;

    if (!va) {
        // Detach successfully
        return true;
    }

    if (!SvmMemory::mapRAM(va, sizeof *header, pa)) {
        // Not even enough room for a valid header
        return false;
    }

    // We read 'last' exactly once, prior to mapping memory.
    header = reinterpret_cast<_SYSBluetoothQueueHeader*>(pa);
    last = header->last;
    unsigned size = last + 1;

    if (!SvmMemory::mapRAM(va, sizeof *header + sizeof(_SYSBluetoothPacket) * size, pa)) {
        // Not enough room for all packets claimed to be part of this queue
        return false;
    }

    queue = reinterpret_cast<_SYSBluetoothQueue*>(pa);
    return true;
}

unsigned BTQueue::count() const
{
    ASSERT(hasQueue());
    unsigned size = last + 1;
    return (queue->header.tail - queue->header.head) % size;
}

_SYSBluetoothPacket *BTQueue::peek()
{
    unsigned size = last + 1;
    unsigned head = queue->header.head % size;
    index = head;
    return &queue->packets[head];
}

void BTQueue::pop()
{
    unsigned head = index + 1;
    if (head > last)
        head = 0;
    queue->header.head = head;
}

_SYSBluetoothPacket *BTQueue::reserve()
{
    unsigned size = last + 1;
    unsigned tail = queue->header.tail % size;
    index = tail;
    return &queue->packets[tail];
}

void BTQueue::commit()
{
    unsigned tail = index + 1;
    if (tail > last)
        tail = 0;
    queue->header.tail = tail;
}
