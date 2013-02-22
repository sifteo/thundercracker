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


uint32_t _SYS_usb_write(const uint8_t *buf, uint32_t len, unsigned timeout)
{
#ifndef SIFTEO_SIMULATOR
    ASSERT(len <= UsbHardware::MAX_PACKET);
#endif

    FlashBlockRef ref;
    SvmMemory::VirtAddr va = reinterpret_cast<SvmMemory::VirtAddr>(buf);
    SvmMemory::PhysAddr pa;

    if (!SvmMemory::mapROData(ref, va, len, pa)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return 0;
    }

#ifdef SIFTEO_SIMULATOR
    return 0;
#else
    return UsbDevice::write(pa, len, timeout);
#endif
}

uint32_t _SYS_usb_counters(_SYSUsbCounters *buffer, uint32_t bufferSize)
{
    if (!SvmMemory::mapRAM(buffer, bufferSize)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return 0;
    }

    const _SYSUsbCounters *counters = 0; //BTProtocol::getCounters();

    unsigned actualSize = MIN(sizeof *counters, bufferSize);
    memset(buffer, 0, bufferSize);
    memcpy(buffer, counters, actualSize);

    return actualSize;
}

} // extern "C"
