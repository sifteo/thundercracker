/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include "svm.h"
#include "svmdebugger.h"
#include "svmdebugpipe.h"
#include "svmcpu.h"
#include "svmruntime.h"
#include "radio.h"
#include "macros.h"

using namespace Svm;
using namespace Svm::Debugger;

#define LOG_PREFIX "SvmDebugger: "

SvmDebugger SvmDebugger::instance;


void SvmDebugger::messageLoop(void *param)
{
    do {
        SvmDebugPipe::DebuggerMsg msg;
        
        while (!SvmDebugPipe::debuggerMsgAccept(msg)) {
            // No command is waiting. If we're halted, block
            // until we get a command. If not, return immediately.            
            if (!instance.stopped)
                return;
            Radio::halt();
        }

        instance.handleMessage(msg);
        SvmDebugPipe::debuggerMsgFinish(msg);

    } while (instance.stopped);
}

bool SvmDebugger::signal(Svm::Debugger::Signals sig)
{
    // Signals are ignored when no debugger is attached.
    if (instance.attached) {
        instance.stopped = sig;
        return true;
    }
    return false;
}

bool SvmDebugger::fault(Svm::FaultCode code)
{
    switch (code) {
        case F_ABORT:   return signal(S_ABORT);
        default:        return signal(S_SEGV);
    }
}

void SvmDebugger::handleMessage(SvmDebugPipe::DebuggerMsg &msg)
{
    /*
     * Handle a single debugger message, with a command and a reply packet.
     * Returns a boolean indicating whether the target should remain stopped.
     */

    // Must be initialized by debuggerMsgAccept().
    ASSERT(msg.replyWords == 0);

    if (msg.cmdWords < 1) {
        LOG((LOG_PREFIX "Empty debug command?\n"));
        return;
    }

    // Any command except detatch will attach the debugger.
    attached = true;

    switch (msg.cmd[0] & M_TYPE_MASK) {
        case M_READ_REGISTERS:      return msgReadRegisters(msg);
        case M_WRITE_REGISTERS:     return msgWriteRegisters(msg);
        case M_READ_RAM:            return msgReadRAM(msg);
        case M_WRITE_RAM:           return msgWriteRAM(msg);
        case M_SIGNAL:              return msgSignal(msg);
        case M_IS_STOPPED:          return msgIsStopped(msg);
        case M_DETACH:              return msgDetach(msg);
    }

    LOG((LOG_PREFIX "Unhandled command 0x%08x\n", msg.cmd[0]));
}

void SvmDebugger::setUserReg(uint32_t r, uint32_t value)
{
    switch (r) {

        // Usermode general-purpose registers
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            SvmCpu::setReg(r, value);
            break;

        // Trusted base pointers. Must point to a valid byte,
        // set to 0 on invalid address.
        case 8:
        case 9: {
            SvmMemory::VirtAddr va = value;
            SvmMemory::PhysAddr pa;
            if (!SvmMemory::mapRAM(va, 1, pa))
                pa = 0;
            SvmCpu::setReg(r, reinterpret_cast<reg_t>(pa));
            break;
        }

        // Trusted stack pointers. Must be aligned, no-op on invalid address.
        case REG_FP:
        case REG_SP: {
            SvmMemory::VirtAddr va = value;
            SvmMemory::PhysAddr pa;
            if ((va & 3) == 0 && SvmMemory::mapRAM(va, 4, pa))
                SvmCpu::setReg(r, reinterpret_cast<reg_t>(pa));
            break;
        }

        // Program counter. Causes a branch when set. Faults on invalid address.
        case REG_PC:
            SvmRuntime::branch(value);
            break;
    }
}

uint32_t SvmDebugger::getUserReg(uint32_t r)
{
    switch (r) {

        // Usermode general-purpose registers, plus read-only CPSR
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case REG_CPSR:
            return SvmCpu::reg(r);

        // Pointer registers: do an address translation, with NULL-passthrough
        case 8:
        case 9:
        case REG_FP:
        case REG_SP: {
            reg_t ptr = SvmCpu::reg(r);
            return ptr ? SvmMemory::physToVirtRAM(ptr) : 0;
        }

        // Reconstruct code address from cache
        case REG_PC:
            return SvmRuntime::reconstructCodeAddr(SvmCpu::reg(REG_PC));

        // Indicate that this register is not mapped to anything physical
        default:
            return (uint32_t) -1;
    }
}

void SvmDebugger::msgReadRegisters(SvmDebugPipe::DebuggerMsg &msg)
{
    uint32_t bitmap = (msg.cmd[0] & M_ARG_MASK) << BITMAP_SHIFT;

    ASSERT(msg.replyWords == 0);
    while (bitmap && msg.replyWords < MAX_REPLY_WORDS) {
        unsigned r = Intrinsic::CLZ(bitmap);
        bitmap &= ~Intrinsic::LZ(r);
        msg.reply[msg.replyWords++] = getUserReg(r);
    }
}

void SvmDebugger::msgWriteRegisters(SvmDebugPipe::DebuggerMsg &msg)
{
    uint32_t bitmap = (msg.cmd[0] & M_ARG_MASK) << BITMAP_SHIFT;
    uint32_t word = 1;

    while (bitmap && word < msg.cmdWords) {
        unsigned r = Intrinsic::CLZ(bitmap);
        bitmap &= ~Intrinsic::LZ(r);
        setUserReg(r, msg.cmd[word++]);
    }
}

void SvmDebugger::msgReadRAM(SvmDebugPipe::DebuggerMsg &msg)
{
    SvmMemory::VirtAddr va = msg.cmd[0] & M_ARG_MASK;
    SvmMemory::PhysAddr pa;
    uint32_t length = msg.cmd[1];

    if (msg.cmdWords < 2 || length > MAX_REPLY_WORDS * sizeof(uint32_t)) {
        LOG((LOG_PREFIX "Malformed READ_RAM command\n"));
        return;
    }

    if (SvmMemory::mapRAM(va, length, pa)) {
        memcpy(msg.reply, pa, length);
        msg.replyWords = (length + sizeof(uint32_t) - 1) / sizeof(uint32_t);
    }
}

void SvmDebugger::msgWriteRAM(SvmDebugPipe::DebuggerMsg &msg)
{
    SvmMemory::VirtAddr va = msg.cmd[0] & M_ARG_MASK;
    SvmMemory::PhysAddr pa;
    uint32_t length = msg.cmd[1];

    if (msg.cmdWords < 3 || length > (msg.cmdWords - 2) * sizeof(uint32_t)) {
        LOG((LOG_PREFIX "Malformed WRITE_RAM command\n"));
        return;
    }

    if (SvmMemory::mapRAM(va, length, pa))
        memcpy(pa, &msg.cmd[2], length);
}

void SvmDebugger::msgSignal(SvmDebugPipe::DebuggerMsg &msg)
{
    signal(Svm::Debugger::Signals(msg.cmd[0] & M_ARG_MASK));
}

void SvmDebugger::msgIsStopped(SvmDebugPipe::DebuggerMsg &msg)
{
    msg.replyWords = 1;
    msg.reply[0] = stopped;
}

void SvmDebugger::msgDetach(SvmDebugPipe::DebuggerMsg &msg)
{
    attached = false;
    stopped = S_RUNNING;
}
