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

    // ensure our check below is meaningful
    if (SysTime::ticks() < SysTime::msTicks(500)) {
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
