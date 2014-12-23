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

/*
 * Syscalls for our userspace Bluetooth API
 */

#include <sifteo/abi.h>
#include "svmruntime.h"
#include "svmmemory.h"
#include "btprotocol.h"

extern "C" {

uint32_t _SYS_bt_isAvailable()
{
    return BTProtocolHardware::isAvailable();
}

uint32_t _SYS_bt_isConnected()
{
    return BTProtocol::isConnected();
}

void _SYS_bt_setPipe(_SYSBluetoothQueue *send, _SYSBluetoothQueue *receive)
{
    if (!BTProtocol::setUserQueues(reinterpret_cast<SvmMemory::VirtAddr>(send),
                                   reinterpret_cast<SvmMemory::VirtAddr>(receive))) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
    }
}

void _SYS_bt_queueWriteHint()
{
    /*
     * A queue write may need to wake up the Bluetooth controller.
     * This is also a placeholder for any future actions we may want
     * to take when userspace writes to its queues.
     */
    BTProtocolHardware::requestProduceData();
}

void _SYS_bt_queueReadHint()
{
    /*
     * A queue read may need to wake up the Bluetooth controller.
     * This is also a placeholder for any future actions we may want
     * to take when userspace reads from its queues.
     */
    BTProtocolHardware::requestProduceData();
}

uint32_t _SYS_bt_counters(_SYSBluetoothCounters *buffer, uint32_t bufferSize)
{
    if (!SvmMemory::mapRAM(buffer, bufferSize)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return 0;
    }

    const _SYSBluetoothCounters *counters = BTProtocol::getCounters();

    unsigned actualSize = MIN(sizeof *counters, bufferSize);
    memset(buffer, 0, bufferSize);
    memcpy(buffer, counters, actualSize);

    return actualSize;
}

void _SYS_bt_advertiseState(const uint8_t *data, uint32_t length)
{
    if (length > _SYS_BT_PACKET_BYTES) {
        return SvmRuntime::fault(F_SYSCALL_PARAM);
    }

    if (!BTProtocol::setUserState(reinterpret_cast<SvmMemory::VirtAddr>(data), length)) {
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);
    }
}


}  // extern "C"
