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
#include "tasks.h"

using namespace Svm;
using namespace Svm::Debugger;

#define LOG_PREFIX "SvmDebugger: "

SvmDebugger SvmDebugger::instance;


void SvmDebugger::messageLoop(void *param)
{
    /*
     * Re-entrancy guard. Normally this isn't something we have to worry
     * about, but an emulated syscall during single-step can call Tasks::work()
     * while we're already in the event loop. This would deadlock us in
     * debuggerMsgAccept, or worse.
     *
     * It isn't sufficient or necessary to put a reentrancy guard on Tasks:work()
     * itself. If we are single-stepping from inside SvmRuntime::breakpoint(),
     * there may not already be a Tasks::work() on the stack above us.
     */
    if (instance.inMessageLoop)
        return;
    instance.inMessageLoop = true;
    instance.messageLoopWork();
    instance.inMessageLoop = false;
}

void SvmDebugger::messageLoopWork()
{
    do {
        SvmDebugPipe::DebuggerMsg msg;
        
        while (!SvmDebugPipe::debuggerMsgAccept(msg)) {
            // No command is waiting. If we're halted, block
            // until we get a command. If not, return immediately.            
            if (!stopped)
                return;
            Radio::halt();
        }

        handleMessage(msg);
        SvmDebugPipe::debuggerMsgFinish(msg);

    } while (stopped);
}

bool SvmDebugger::signal(Svm::Debugger::Signals sig)
{
    // Signals are ignored when no debugger is attached.
    if (!instance.attached)
        return false;

    // Any signal (including breakpoints) will clear our single-step breakpoint
    instance.setStepBreakpoint(0);

    // Make sure the debugger event loop will run, and tell it to stop/run.
    instance.stopped = sig;
    Tasks::setPending(Tasks::Debugger);
    return true;
}

bool SvmDebugger::fault(Svm::FaultCode code)
{
    switch (code) {
        case F_ABORT:
            return signal(S_ABORT);

        case F_BAD_CODE_ADDRESS:
        case F_CODE_FETCH:
        case F_CODE_ALIGNMENT:
        case F_CPU_SIM:
        case F_RESERVED_SVC:
        case F_RESERVED_ADDROP:
            return signal(S_ILL);

        case F_LOAD_ALIGNMENT:
        case F_STORE_ALIGNMENT:
            return signal(S_BUS);

        default:
            return signal(S_SEGV);
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

    // Any command except detach will attach the debugger.
    attached = true;

    switch (msg.cmd[0] & M_TYPE_MASK) {
        case M_READ_REGISTERS:      return msgReadRegisters(msg);
        case M_WRITE_REGISTERS:     return msgWriteRegisters(msg);
        case M_READ_RAM:            return msgReadRAM(msg);
        case M_WRITE_RAM:           return msgWriteRAM(msg);
        case M_SIGNAL:              return msgSignal(msg);
        case M_IS_STOPPED:          return msgIsStopped(msg);
        case M_DETACH:              return msgDetach(msg);
        case M_SET_BREAKPOINTS:     return msgSetBreakpoints(msg);
        case M_STEP:                return msgStep(msg);
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
        // Note that it isn't possible to set the PC to a non-bundle aligned
        // address. This makes it important to avoid branching if the PC is
        // being set to the same value that we last reported for REG_PC.
        case REG_PC:
            if (value != getUserReg(REG_PC))
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
    // Clear all breakpoints
    memset(breakpoints, 0, sizeof breakpoints);
    breakpointsChanged();

    // Run!
    attached = false;
    stopped = S_RUNNING;
}

void SvmDebugger::msgSetBreakpoints(SvmDebugPipe::DebuggerMsg &msg)
{
    /*
     * Change any number of breakpoint locations, specifying the
     * affected breakpoint slots using a bitmap. This stores the
     * new breakpoints, then invalidates the flash cache. We apply
     * breakpoints while paging code back in.
     */

    uint32_t bitmap = (msg.cmd[0] & M_ARG_MASK) << BITMAP_SHIFT;
    uint32_t word = 1;

    while (bitmap && word < msg.cmdWords) {
        unsigned i = Intrinsic::CLZ(bitmap);
        bitmap &= ~Intrinsic::LZ(i);

        if (i < NUM_USER_BREAKPOINTS) {
            SvmMemory::VirtAddr va = msg.cmd[word++];
            breakpoints[i] = SvmMemory::virtToFlashAddr(va);
        }
    }
    
    breakpointsChanged();
}
    
void SvmDebugger::breakpointsChanged()
{
    // Recalculate breakpointMap. It contains only the live breakpoints.
    uint32_t bitmap = 0;

    for (unsigned i = 0; i < NUM_TOTAL_BREAKPOINTS; i++)
        if (breakpoints[i] != 0)
            bitmap |= Intrinsic::LZ(i);
    breakpointMap = bitmap;

    FlashBlock::invalidate();
}

void SvmDebugger::patchFlashBlock(uint32_t blockAddr, uint8_t *data)
{
    /*
     * When a flash block is paging in, patch it to insert breakpoints.
     */

    uint32_t bitmap = instance.breakpointMap;
    while (bitmap) {
        unsigned i = Intrinsic::CLZ(bitmap);
        bitmap &= ~Intrinsic::LZ(i);
        uint32_t addr = instance.breakpoints[i];
        uint32_t mask = FlashBlock::BLOCK_SIZE - 1;

        if ((addr & ~mask) == blockAddr) {
            uint32_t offset = addr & mask;
            breakpointPatch(data, offset);
        }
    }
}

void SvmDebugger::breakpointPatch(uint8_t *data, uint32_t offset)
{
    /*
     * Insert a breakpoint in the block cache, at the specified offset.
     * We must ensure that the offset is properly aligned, and that we
     * aren't splitting a 32-bit instruction.
     *
     * Note that this will occur prior to SvmValidator running, so we need
     * to be sure to produce valid code. For example, if we're patching
     * a 32-bit instruction, we need to make sure the second halfword is
     * also a valid instruction. In our case, we put in two back-to-back
     * breakpoints.
     */

    if (offset & 1) {
        LOG((LOG_PREFIX "Breakpoint is not halfword aligned\n"));
        return;
    }

    // Point to the first word in the bundle
    uint16_t *pBundle = reinterpret_cast<uint16_t*>(data + (offset & ~3));

    if (offset & 3) {
        // Not bundle-aligned. See if this is okay!

        if (instructionSize(pBundle[0]) == InstrBits32) {
            LOG((LOG_PREFIX "Breakpoint would split a 32-bit instruction; not allowed\n"));
            return;
        }

        // Simple 16-bit breakpoint in second word.
        pBundle[1] = BreakpointInstr;

    } else {
        // Bundle-aligned. If this is a 32-bit instruction, patch both halves

        if (instructionSize(pBundle[0]) == InstrBits32)
            pBundle[0] = pBundle[1] = BreakpointInstr;
        else
            pBundle[0] = BreakpointInstr;
    }
}

void SvmDebugger::setStepBreakpoint(SvmMemory::VirtAddr va)
{
    uint32_t addr = va ? SvmMemory::virtToFlashAddr(va) : 0;
    if (addr != breakpoints[STEP_BREAKPOINT]) {
        breakpoints[STEP_BREAKPOINT] = addr;
        breakpointsChanged();
    }
}

void SvmDebugger::msgStep(SvmDebugPipe::DebuggerMsg &msg)
{
    /*
     * Single-step. PC points to the instruction we'll be executing.
     */

    // Single-stepping starts out by going back into S_RUNNING mode.
    stopped = S_RUNNING;

    // Read the current PC
    SvmMemory::VirtAddr pc = getUserReg(REG_PC);

    // Fetch the next instruction. If this fails, it's a fault.
    FlashBlockRef ref;
    uint16_t instr;
    if (!SvmMemory::copyROData(ref, SvmMemory::PhysAddr(&instr), pc, sizeof instr))
        return SvmRuntime::fault(F_CODE_FETCH);

    if (instructionSize(instr) == InstrBits32) {
        // A 32-bit instruction. No valid 32-bit instructions modify control
        // flow, so place a breakpoint immediately afterwards.
        return setStepBreakpoint(pc + 4);
    }

    if ((instr & SvcMask) == SvcTest) {
        // SVCs are emulated during single-step.
        SvmCpu::setReg(REG_PC, SvmCpu::reg(REG_PC) + 2);
        SvmDebugger::signal(Svm::Debugger::S_TRAP);
        return SvmRuntime::svc(uint8_t(instr));
    }

    if ((instr & UncondBranchMask) == UncondBranchTest) {
        // Emulated unconditional branch
        return setStepBreakpoint(branchTargetB(instr, pc + 2));
    }

    if ((instr & CondBranchMask) == CondBranchTest) {
        // Emulated conditional branch
        return setStepBreakpoint(branchTargetCondB(instr, pc + 2, SvmCpu::reg(REG_CPSR)));
    }

    if ((instr & CompareBranchMask) == CompareBranchTest) {
        // Emulated compare-and-branch
        return setStepBreakpoint(branchTargetCBZ_CBNZ(instr, pc + 2,
            SvmCpu::reg(REG_CPSR), SvmCpu::reg(instr & 7)));
    }
    
    // All other 16-bit native instructions don't modify control flow.
    return setStepBreakpoint(pc + 2);
}
