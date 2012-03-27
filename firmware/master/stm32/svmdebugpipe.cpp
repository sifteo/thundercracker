/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "svm.h"
#include "svmdebugpipe.h"
#include "svmruntime.h"
using namespace Svm;


void SvmDebugPipe::setSymbolSourceELF(const FlashRange &elf)
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
    // XXX: Stub
    static uint32_t buffer[7];
    return buffer;
}

void SvmDebugPipe::logCommit(SvmLogTag tag, uint32_t *buffer, uint32_t bytes)
{
    // XXX: Stub
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
