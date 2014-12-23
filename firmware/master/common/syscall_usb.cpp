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
