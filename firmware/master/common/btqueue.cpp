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
