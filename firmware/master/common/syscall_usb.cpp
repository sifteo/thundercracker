/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for USB operations.
 */

#include <sifteo/abi.h>
#include "svmmemory.h"
#include "svmruntime.h"
#include "macros.h"
#include "usbprotocol.h"

#ifndef SIFTEO_SIMULATOR
#include "usb/usbdevice.h"
#include "usb/usbhardware.h"
#include "powermanager.h"
#endif

extern "C" {

uint32_t _SYS_usb_isConnected()
{
#ifdef SIFTEO_SIMULATOR
    return false;
#else
    return (PowerManager::state() == PowerManager::UsbPwr);
#endif
}

void _SYS_usb_setPipe(_SYSUsbQueue *send, _SYSUsbQueue *receive)
{
    if (!USBProtocol::setUserQueues(reinterpret_cast<SvmMemory::VirtAddr>(send),
                                   reinterpret_cast<SvmMemory::VirtAddr>(receive))) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
    }
}

void _SYS_usb_queueWriteHint()
{
    USBProtocol::requestUserINPacket();
}

uint32_t _SYS_usb_counters(_SYSUsbCounters *buffer, uint32_t bufferSize)
{
    if (!SvmMemory::mapRAM(buffer, bufferSize)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return 0;
    }

#ifdef SIFTEO_SIMULATOR
    return 0;
#else
    const _SYSUsbCounters *counters = USBProtocol::getCounters();

    unsigned actualSize = MIN(sizeof *counters, bufferSize);
    memset(buffer, 0, bufferSize);
    memcpy(buffer, counters, actualSize);

    return actualSize;
#endif
}

} // extern "C"
