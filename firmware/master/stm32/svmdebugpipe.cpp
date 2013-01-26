/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "svm.h"
#include "svmdebugpipe.h"
#include "svmruntime.h"
#include "powermanager.h"
#include "usbprotocol.h"
#include "usb/usbdevice.h"

using namespace Svm;

static USBProtocolMsg logMsg;

void SvmDebugPipe::init()
{
    // Do nothing (Only used in simulation)
}

void SvmDebugPipe::setSymbolSource(const Elf::Program &program)
{
    // Do nothing (Only used in simulation)
}

bool SvmDebugPipe::fault(FaultCode code)
{
    // XXX: Stub
    return false;
}

uint32_t *SvmDebugPipe::logReserve(SvmLogTag tag)
{
    /*
     * Ensure we're ready to write a USB packet,
     * and provide the caller with a pointer to our
     * protocol msg payload to begin populating.
     */

    logMsg.init(USBProtocol::Logger);
    return reinterpret_cast<uint32_t*>(&logMsg.payload[0]);
}

void SvmDebugPipe::logCommit(SvmLogTag tag, uint32_t *buffer, uint32_t bytes)
{
    /*
     * Time to write out a log message.
     * If we're not connected via USB, or the host hasn't been
     * listening to us for a while, don't bother.
     */

    if (PowerManager::state() != PowerManager::UsbPwr) {
        return;
    }

    if (SysTime::ticks() - UsbDevice::lastINActivity() > SysTime::msTicks(250)) {
        return;
    }

    logMsg.header |= tag.getValue();
    logMsg.len += (LOG_BUFFER_WORDS + bytes);
    UsbDevice::write(logMsg.bytes, logMsg.len, 100);
}

bool SvmDebugPipe::debuggerMsgAccept(DebuggerMsg &msg)
{
    // XXX: Stub
    return false;
}

void SvmDebugPipe::debuggerMsgFinish(DebuggerMsg &msg)
{
    // XXX: Stub
}
