/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "svm.h"
#include "svmdebug.h"
#include "svmruntime.h"
using namespace Svm;


void SvmDebug::fault(FaultCode code)
{
    // XXX: Stub
    while (1);
}

uint32_t *SvmDebug::logReserve(SvmLogTag tag)
{
    // XXX: Stub
    static uint32_t buffer[7];
    return buffer;
}

void SvmDebug::logCommit(SvmLogTag tag, uint32_t *buffer)
{
    // XXX: Stub
}

void SvmDebug::setSymbolSourceELF(const FlashRange &elf)
{
    // Only used in simulation
}

