/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for our userspace Bluetooth API
 */

#include <sifteo/abi.h>

extern "C" {

uint32_t _SYS_bt_isAvailable()
{
    return true;
}

uint32_t _SYS_bt_isConnected()
{
    return true;
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
