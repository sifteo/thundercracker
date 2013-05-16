/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for our userspace Bluetooth API
 */

#include <sifteo/abi.h>
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

void _SYS_bt_advertiseState(const uint8_t *data, uint32_t length)
{
}

void _SYS_bt_setPipe(_SYSBluetoothQueue *send, _SYSBluetoothQueue *receive)
{
}

void _SYS_bt_queueWriteHint()
{
}

void _SYS_bt_queueReadHint()
{
}

}  // extern "C"
