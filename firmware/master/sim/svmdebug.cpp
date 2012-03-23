/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <string.h>
#include "svm.h"
#include "svmdebug.h"
#include "svmmemory.h"
#include "svmruntime.h"
#include "elfdefs.h"
#include "elfdebuginfo.h"
#include "logdecoder.h"
#include "gdbserver.h"
using namespace Svm;

static ELFDebugInfo gELFDebugInfo;


static const char* faultStr(FaultCode code)
{
    switch (code) {
    case F_STACK_OVERFLOW:      return "Stack allocation failure";
    case F_BAD_STACK:           return "Validation-time stack address error";
    case F_BAD_CODE_ADDRESS:    return "Branch-time code address error";
    case F_BAD_SYSCALL:         return "Unsupported syscall number";
    case F_LOAD_ADDRESS:        return "Runtime load address error";
    case F_STORE_ADDRESS:       return "Runtime store address error";
    case F_LOAD_ALIGNMENT:      return "Runtime load alignment error";
    case F_STORE_ALIGNMENT:     return "Runtime store alignment error";
    case F_CODE_FETCH:          return "Runtime code fetch error";
    case F_CODE_ALIGNMENT:      return "Runtime code alignment error";
    case F_CPU_SIM:             return "Unhandled ARM instruction in sim";
    case F_RESERVED_SVC:        return "Reserved SVC encoding";
    case F_RESERVED_ADDROP:     return "Reserved ADDROP encoding";
    case F_ABORT:               return "User call to _SYS_abort";
    case F_LONG_STACK_LOAD:     return "Bad address in long stack LDR addrop";
    case F_LONG_STACK_STORE:    return "Bad address in long stack STR addrop";
    case F_PRELOAD_ADDRESS:     return "Bad address for async preload";
    case F_RETURN_FRAME:        return "Bad saved FP value detected during return";
    case F_LOG_FETCH:           return "Memory fault while fetching _SYS_log data";
    default:                    return "unknown error";
    }
}
    
void SvmDebug::fault(FaultCode code)
{
    uint32_t pcVA = SvmRuntime::reconstructCodeAddr(SvmCpu::reg(REG_PC));
    std::string pcName = formatAddress(pcVA);

    LOG(("***\n"
         "*** (>\")> --[ Uh oh!  A VM Fault! ]\n"
         "***\n"
         "***   Code %d (%s)\n"
         "***\n"
         "***   PC: va=%08x pa=%p%s at %s\n"
         "***   SP: va=%08x pa=%p%s\n"
         "***  GPR: %08x %08x %08x %08x\n"
         "***       %08x %08x %08x %08x\n"
         "***\n"
         "*** Stack Backtrace:\n"
         "***\n",
         code, faultStr(code),
 
         pcVA,
         reinterpret_cast<void*>(SvmCpu::reg(REG_PC)),
         SvmMemory::isAddrValid(SvmCpu::reg(REG_PC)) ? "" : " (INVALID)",
         pcName.c_str(),

         (unsigned)SvmMemory::physToVirtRAM(
             reinterpret_cast<SvmMemory::PhysAddr>(SvmCpu::reg(REG_SP))),
         reinterpret_cast<void*>(SvmCpu::reg(REG_SP)),
         SvmMemory::isAddrValid(SvmCpu::reg(REG_SP)) ? "" : " (INVALID)",

         (unsigned) SvmCpu::reg(0),
         (unsigned) SvmCpu::reg(1),
         (unsigned) SvmCpu::reg(2),
         (unsigned) SvmCpu::reg(3),
         (unsigned) SvmCpu::reg(4),
         (unsigned) SvmCpu::reg(5),
         (unsigned) SvmCpu::reg(6),
         (unsigned) SvmCpu::reg(7)
    ));

    SvmMemory::VirtAddr fpVA = SvmCpu::reg(REG_FP);
    SvmMemory::PhysAddr fpPA;
    while (SvmMemory::mapRAM(fpVA, sizeof(CallFrame), fpPA)) {
        CallFrame *frame = reinterpret_cast<CallFrame*>(fpPA);
        std::string symbolName = formatAddress(frame->pc);

        LOG(("***  [%08x] pc=%08x  %s\n",
            (unsigned)SvmMemory::physToVirtRAM(fpPA),
            frame->pc, symbolName.c_str()));

        fpVA = frame->fp;
    }

    LOG(("***\n"));
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

void SvmDebug::logCommit(SvmLogTag tag, uint32_t *buffer, uint32_t bytes)
{
    // On real hardware, log decoding would be deferred to the host machine.
    // In simulation, we can decode right away. Note that we use the raw Flash
    // interface instead of going through the cache, since we don't want
    // debug log decoding to affect caching behavior.

    uint32_t decodedSize = LogDecoder::decode(gELFDebugInfo, tag, buffer);
    ASSERT(decodedSize == bytes);
}

std::string SvmDebug::formatAddress(uint32_t address)
{
    return gELFDebugInfo.formatAddress(address);
}

std::string SvmDebug::formatAddress(void *address)
{
    return gELFDebugInfo.formatAddress(SvmMemory::physToVirtRAM((uint8_t*)address));
}

bool SvmDebug::debuggerMsgAccept(SvmDebug::DebuggerMsg &msg)
{
    // XXX
    return false;
}

void SvmDebug::debuggerMsgFinish(SvmDebug::DebuggerMsg &msg)
{
    // XXX
}

static uint32_t debuggerMsgCallback(const uint32_t *cmd,
    uint32_t cmdWords, uint32_t *reply)
{
    // XXX
    return 0;
}

void SvmDebug::setSymbolSourceELF(const FlashRange &elf)
{
    gELFDebugInfo.init(elf);
    GDBServer::setDebugInfo(&gELFDebugInfo);
    GDBServer::setMessageCallback(debuggerMsgCallback);
}
