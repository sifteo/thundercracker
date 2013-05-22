/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
    // Implement me! Copy the advertised state in a system buffer owned by BTProtocol.
}


}  // extern "C"
