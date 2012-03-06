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
    LOG(("***\n"
         "*** VM FAULT code %d\n"
         "***\n"
         "***   PC: %08x SP: %"PRIxPTR"\n"
         "***  GPR: %08x %08x %08x %08x\n"
         "***       %08x %08x %08x %08x\n"
         "***\n",
         code,
         SvmRuntime::reconstructCodeAddr(),
         SvmCpu::reg(REG_SP),
         (unsigned) SvmCpu::reg(0), (unsigned) SvmCpu::reg(1),
         (unsigned) SvmCpu::reg(2), (unsigned) SvmCpu::reg(3),
         (unsigned) SvmCpu::reg(4), (unsigned) SvmCpu::reg(5),
         (unsigned) SvmCpu::reg(6), (unsigned) SvmCpu::reg(7)));

    SvmRuntime::exit();
}

uint32_t *SvmDebug::logReserve(SvmLogTag tag)
{
    // On real hardware, we'll be writing directly into a USB ring buffer.
    // On simulation, we can just stow the parameters in a temporary global
    // buffer, and decode them to stdout immediately.

    static uint32_t buffer[7];
    return buffer;
}

void SvmDebug::logCommit(SvmLogTag tag, uint32_t *buffer)
{
    // On real hardware, log decoding would be deferred to the host machine.
    // In simulation, we can decode right away.
}

void SvmDebug::setSymbolSourceELF(const FlashRange &elf)
{
    // Search for the debug symbols, etc.
}


