/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "svmruntime.h"
#include "flash_blockcache.h"
#include "svm.h"
#include "svmmemory.h"
#include "svmdebugpipe.h"
#include "svmdebugger.h"
#include "svmloader.h"
#include "event.h"
#include "tasks.h"
#include "panic.h"
#include "homebutton.h"
#include "cubeslots.h"

#include <math.h>
#include <sifteo/abi.h>

typedef uint64_t (*SvmSyscall)(reg_t p0, reg_t p1, reg_t p2, reg_t p3,
                               reg_t p4, reg_t p5, reg_t p6, reg_t p7);

// Library function aliases, used by syscall-table on hardware only.
#ifdef SIFTEO_SIMULATOR
#   define SYS_ALIAS(_sysName, _libName)   _sysName
#else
#   define SYS_ALIAS(_sysName, _libName)   _libName
#endif

#include "syscall-table.def"

using namespace Svm;

FlashBlockRef SvmRuntime::codeBlock;
FlashBlockRef SvmRuntime::dataBlock;
SvmMemory::PhysAddr SvmRuntime::stackLimit;
reg_t SvmRuntime::eventFrame;
bool SvmRuntime::eventDispatchFlag;


void SvmRuntime::run(uint32_t entryFunc, const StackInfo &stack)
{
    UART(("Entering SVM.\r\n"));
    initStack(stack);
    SvmCpu::run(mapSP(stack.top - getSPAdjustBytes(entryFunc)),
                mapBranchTarget(entryFunc));
}

void SvmRuntime::exec(uint32_t entryFunc, const StackInfo &stack)
{
    // Unset base pointers
    validate(0);

    // Escape from any active events
    eventFrame = 0;
    eventDispatchFlag = false;

    // Reset stack limits
    initStack(stack);

    // Zero all GPRs
    for (unsigned i = 0; i < 8; ++i)
        SvmCpu::setReg(i, 0);

    // We're back in main() now, so set FP accordingly
    SvmCpu::setReg(REG_FP, 0);

    setSP(stack.top - getSPAdjustBytes(entryFunc));
    branch(entryFunc);
}

void SvmRuntime::initStack(const StackInfo &stack)
{
    if (!SvmMemory::mapRAM(stack.limit, 0, stackLimit))
        SvmRuntime::fault(F_BAD_STACK);

#ifdef SIFTEO_SIMULATOR
    ASSERT(SvmMemory::mapRAM(stack.top, (uint32_t)0, topOfStackPA));
    stackLowWaterMark = topOfStackPA;
#endif
}

void SvmRuntime::dumpRegister(PanicMessenger &msg, unsigned reg)
{
    reg_t value = SvmCpu::reg(reg);
    SvmMemory::squashPhysicalAddr(value);
    msg << uint32_t(value);
}

void SvmRuntime::fault(FaultCode code)
{
    // Try to find a handler for this fault. If nobody steps up,
    // force the system to exit.

    // First priority: an attached debugger
    if (SvmDebugger::fault(code))
        return;
    
    // Next priority: Log the fault in a platform-specific way
    if (SvmDebugPipe::fault(code))
        return;

    /* 
     * Unhandled fault; panic!
     *
     * Draw a message to one enabled cube, and exit after a home-button press.
     */

    _SYSCubeIDVector cubes = CubeSlots::vecEnabled;
    if (cubes) {

        PanicMessenger msg;
        msg.init(0x10000);

        // User-facing description
        msg.at(1,1) << "Oh no!";
        msg.at(1,2) << "Press button";
        msg.at(1,3) << "to continue.";

        // Getting more developer-oriented now... fault description
        msg.at(1,5) << "In Volume<" << uint8_t(SvmLoader::getRunningVolume().block.code) << ">";
        msg.at(1,6) << "Fault 0x" << uint8_t(code) << ":";
        msg.at(1,7) << faultString14(code);

        // Begin the "Wall of text" register dump, after one blank line
        uint32_t pcVA = SvmRuntime::reconstructCodeAddr(SvmCpu::reg(REG_PC));
        msg.at(1,9) << "PC: " << pcVA;

        dumpRegister(msg.at(1,10) << "SP: ", REG_SP);

        // Only room for first 4 GPRs
        for (unsigned r = 0; r < 4; r++)
            dumpRegister(msg.at(1,11+r) << 'r' << char('0' + r) << ": ", r);

        // Paint on first enabled cube
        msg.paint(Intrinsic::LZ(cubes));

        // Wait for home button press and release
        while (!HomeButton::isPressed())
            Tasks::idle();
        while (HomeButton::isPressed())
            Tasks::idle();
    }

    // Exit with an error (Back to the launcher)
    SvmLoader::exit(true);
}

void SvmRuntime::call(reg_t addr)
{
    // Allocate a CallFrame for this function
    adjustSP(-(int)(sizeof(CallFrame) / sizeof(uint32_t)));
    CallFrame *fp = reinterpret_cast<CallFrame*>(SvmCpu::reg(REG_SP));

    reg_t sFP = SvmCpu::reg(REG_FP);
    reg_t sR2 = SvmCpu::reg(2);
    reg_t sR3 = SvmCpu::reg(3);
    reg_t sR4 = SvmCpu::reg(4);
    reg_t sR5 = SvmCpu::reg(5);
    reg_t sR6 = SvmCpu::reg(6);
    reg_t sR7 = SvmCpu::reg(7);

    // Because this is a store to RAM, on simulated builds
    // we may need to squash 64-bit pointers.
    SvmMemory::squashPhysicalAddr(sFP);
    SvmMemory::squashPhysicalAddr(sR2);
    SvmMemory::squashPhysicalAddr(sR3);
    SvmMemory::squashPhysicalAddr(sR4);
    SvmMemory::squashPhysicalAddr(sR5);
    SvmMemory::squashPhysicalAddr(sR6);
    SvmMemory::squashPhysicalAddr(sR7);

    fp->pc = reconstructCodeAddr(SvmCpu::reg(REG_PC));
    fp->fp = sFP;
    fp->r2 = sR2;
    fp->r3 = sR3;
    fp->r4 = sR4;
    fp->r5 = sR5;
    fp->r6 = sR6;
    fp->r7 = sR7;

    // This is now the current frame
    SvmCpu::setReg(REG_FP, reinterpret_cast<reg_t>(fp));

    TRACING_ONLY({
        LOG(("CALL: %08x, sp-%u, Saving frame %p: pc=%08x fp=%08x r2=%08x "
            "r3=%08x r4=%08x r5=%08x r6=%08x r7=%08x\n",
            (unsigned)(addr & 0xfffffc), getSPAdjustWords(addr),
            fp, fp->pc, fp->fp, fp->r2, fp->r3, fp->r4, fp->r5, fp->r6, fp->r7));
    });

    enterFunction(addr);
}

void SvmRuntime::tailcall(reg_t addr)
{
    // Equivalent to a call() followed by a ret(), but without
    // allocating a new CallFrame on the stack.
    
    reg_t fp = SvmCpu::reg(REG_FP);

    if (fp) {
        // Restore stack to bottom of the saved frame
        setSP(fp);
    } else {
        // Tailcall from main(), restore stack to top
        resetSP();
    }

    TRACING_ONLY({
        LOG(("TAILCALL: %08x, sp-%u, Keeping frame %p\n",
            (unsigned)(addr & 0xfffffc), getSPAdjustWords(addr),
            reinterpret_cast<void*>(fp)));
    });

    enterFunction(addr);
}

void SvmRuntime::enterFunction(reg_t addr)
{
    // Allocate stack space for this function, and enter it
    adjustSP(-(int)getSPAdjustWords(addr));
    branch(addr);
}

void SvmRuntime::ret(unsigned actions)
{
    reg_t regFP = SvmCpu::reg(REG_FP);
    CallFrame *fp = reinterpret_cast<CallFrame*>(regFP);

    if (!fp) {
        // No more functions on the stack. Return from main() is exit().
        if (actions & RET_EXIT)
            SvmLoader::exit();
        return;
    }

    /*
     * Restore the saved frame. Note that REG_FP, and therefore 'fp',
     * are trusted, however the saved value at fp->fp needs to be validated
     * before it can be loaded into the trusted FP register.
     */

    TRACING_ONLY({
        LOG(("RET: Restoring frame %p: pc=%08x fp=%08x r2=%08x "
            "r3=%08x r4=%08x r5=%08x r6=%08x r7=%08x\n",
            fp, fp->pc, fp->fp, fp->r2, fp->r3, fp->r4, fp->r5, fp->r6, fp->r7));
    });

    SvmMemory::VirtAddr fpVA = fp->fp;
    SvmMemory::PhysAddr fpPA;
    if (fpVA) {
        if (!SvmMemory::mapRAM(fpVA, sizeof(CallFrame), fpPA)) {
            SvmRuntime::fault(F_RETURN_FRAME);
            return;
        }
    } else {
        // Zero is a legal FP value, used as a sentinel.
        fpPA = 0;
    }

    if (actions & RET_BRANCH) {
        branch(fp->pc);
    }

    if (actions & RET_RESTORE_REGS) {
        SvmCpu::setReg(2, fp->r2);
        SvmCpu::setReg(3, fp->r3);
        SvmCpu::setReg(4, fp->r4);
        SvmCpu::setReg(5, fp->r5);
        SvmCpu::setReg(6, fp->r6);
        SvmCpu::setReg(7, fp->r7);
    }

    if (actions & RET_POP_FRAME) {
        SvmCpu::setReg(REG_FP, reinterpret_cast<reg_t>(fpPA));
        setSP(reinterpret_cast<reg_t>(fp + 1));

        // If we're returning from an event handler, see if we still need
        // to dispatch any other pending events.
        if (eventFrame == regFP) {
            eventFrame = 0;
            dispatchEventsOnReturn();
        }
    }
}

#ifndef SIFTEO_SIMULATOR
    #include "sampleprofiler.h"
#endif

void SvmRuntime::svc(uint8_t imm8)
{
    #ifndef SIFTEO_SIMULATOR
        SampleProfiler::SubSystem s = SampleProfiler::subsystem();
        SampleProfiler::setSubsystem(SampleProfiler::SVCISR);
    #endif

    if ((imm8 & (1 << 7)) == 0) {
        if (imm8 == 0)
            ret();
        else
            svcIndirectOperation(imm8);

    } else if ((imm8 & (0x3 << 6)) == (0x2 << 6)) {
        uint8_t syscallNum = imm8 & 0x3f;
        syscall(syscallNum);
        postSyscallWork();

    } else if ((imm8 & (0x7 << 5)) == (0x6 << 5)) {
        int imm5 = imm8 & 0x1f;
        adjustSP(-imm5);

    } else {
        uint8_t sub = (imm8 >> 3) & 0x1f;
        unsigned r = imm8 & 0x7;

        switch (sub) {
        case 0x1c:  // 0b11100
            validate(SvmCpu::reg(r));
            break;

        case 0x1d:  // 0b11101
            if (r)
                fault(F_RESERVED_SVC);
            else
                breakpoint();
            break;

        case 0x1e:  // 0b11110
            call(SvmCpu::reg(r));
            break;

        case 0x1f:  // 0b11111
            tailcall(SvmCpu::reg(r));
            break;

        default:
            fault(F_RESERVED_SVC);
            break;
        }
    }

    #ifndef SIFTEO_SIMULATOR
        SampleProfiler::setSubsystem(s);
    #endif
}

void SvmRuntime::svcIndirectOperation(uint8_t imm8)
{
    // Should be checked by the validator
    ASSERT(imm8 < FlashBlock::BLOCK_SIZE / sizeof(uint32_t));

    uint32_t *blockBase = reinterpret_cast<uint32_t*>(codeBlock->getData());
    uint32_t literal = blockBase[imm8];

    if ((literal & CallMask) == CallTest) {
        call(literal);
    }
    else if ((literal & TailCallMask) == TailCallTest) {
        tailcall(literal);
    }
    else if ((literal & IndirectSyscallMask) == IndirectSyscallTest) {
        unsigned imm15 = (literal >> 16) & 0x3ff;
        syscall(imm15);
        postSyscallWork();
    }
    else if ((literal & TailSyscallMask) == TailSyscallTest) {
        unsigned imm15 = (literal >> 16) & 0x3ff;
        tailSyscall(imm15);
        postSyscallWork();
    }
    else if ((literal & AddropMask) == AddropTest) {
        unsigned opnum = (literal >> 24) & 0x1f;
        addrOp(opnum, literal & 0xffffff);
    }
    else if ((literal & AddropFlashMask) == AddropFlashTest) {
        unsigned opnum = (literal >> 24) & 0x1f;
        addrOp(opnum, SvmMemory::SEGMENT_0_VA + (literal & 0xffffff));
    }
    else {
        SvmRuntime::fault(F_RESERVED_SVC);
    }
}

void SvmRuntime::addrOp(uint8_t opnum, reg_t address)
{
    switch (opnum) {
    case 0:
        branch(address);
        break;

    case 1:
        if (!SvmMemory::preload(address))
            SvmRuntime::fault(F_PRELOAD_ADDRESS);
        break;

    case 2:
        validate(address);
        break;

    case 3:
        adjustSP(-(int)address);
        break;

    case 4:
        longSTRSP((address >> 21) & 7, address & 0x1FFFFF);
        break;

    case 5:
        longLDRSP((address >> 21) & 7, address & 0x1FFFFF);
        break;

    default:
        SvmRuntime::fault(F_RESERVED_ADDROP);
        break;
    }
}

void SvmRuntime::validate(reg_t address)
{
    /*
     * Map 'address' as either a flash or RAM address, and set the
     * base pointer registers r8-9 appropriately.
     */

    // Mask off high bits, for 64-bit Siftulator builds
    address = (uint32_t) address;

    SvmMemory::PhysAddr bro, brw;
    SvmMemory::validateBase(dataBlock, address, bro, brw);

    SvmCpu::setReg(8, reinterpret_cast<reg_t>(bro));
    SvmCpu::setReg(9, reinterpret_cast<reg_t>(brw));
}

void SvmRuntime::syscall(unsigned num)
{
    // syscall calling convention: 8 params, as provided by r0-r7,
    // and return up to 64 bits in r0-r1. Note that the return value is never
    // a system pointer, so for that purpose we treat return values as 32-bit
    // registers.

    if (num >= sizeof SyscallTable / sizeof SyscallTable[0]) {
        SvmRuntime::fault(F_BAD_SYSCALL);
        return;
    }
    SvmSyscall fn = SyscallTable[num];
    if (!fn) {
        SvmRuntime::fault(F_BAD_SYSCALL);
        return;
    }

    TRACING_ONLY({
        LOG(("SYSCALL: enter _SYS_%d(%p, %p, %p, %p, %p, %p, %p, %p)\n",
            num,
            reinterpret_cast<void*>(SvmCpu::reg(0)),
            reinterpret_cast<void*>(SvmCpu::reg(1)),
            reinterpret_cast<void*>(SvmCpu::reg(2)),
            reinterpret_cast<void*>(SvmCpu::reg(3)),
            reinterpret_cast<void*>(SvmCpu::reg(4)),
            reinterpret_cast<void*>(SvmCpu::reg(5)),
            reinterpret_cast<void*>(SvmCpu::reg(6)),
            reinterpret_cast<void*>(SvmCpu::reg(7))));
    });

    uint64_t result = fn(SvmCpu::reg(0), SvmCpu::reg(1),
                         SvmCpu::reg(2), SvmCpu::reg(3),
                         SvmCpu::reg(4), SvmCpu::reg(5),
                         SvmCpu::reg(6), SvmCpu::reg(7));

    uint32_t result0 = result;
    uint32_t result1 = result >> 32;

    TRACING_ONLY({
        LOG(("SYSCALL: leave _SYS_%d() -> %x:%x\n",
            num, result1, result0));
    });

    SvmCpu::setReg(0, result0);
    SvmCpu::setReg(1, result1);
}

void SvmRuntime::tailSyscall(unsigned num)
{
    /*
     * Tail syscalls incorporate a normal system call plus a return.
     * Userspace doesn't care what order these two things come in, but
     * we have a couple of conflicting constraints:
     *
     *   1. During syscall execution, we must already have a proper
     *      PC value set. The instruction immediately after a tail syscall
     *      may not be valid, and definitely won't be the next instruction
     *      to execute. So, we need to branch to the return address first.
     *
     *      (This requirement stems from single-stepping support, where
     *      a blocking syscall like Paint may enter the debugger event loop)
     *
     *   2. The syscall needs parameters from the original GPRs, so we can't
     *      have restored the caller's registers already.
     *
     * Therefore, we split the ret() into two pieces. Branch before syscall,
     * everything else after.
     */

    ret(RET_BRANCH);
    syscall(num);
    ret(RET_ALL ^ RET_BRANCH);
}

void SvmRuntime::postSyscallWork()
{
    /*
     * Deferred work items that must be handled after a syscall is fully
     * done, the return values have been stored, and we're ready to return.
     *
     * This is work that could happen at the end of every svc(), but
     * for performance reasons we only do it after syscalls.
     */

    // Poll for pending userspace tasks on our way up. This is akin to a
    // deferred procedure call (DPC) in Win32.
    Tasks::work();
    
    // Event dispatch is requested by certain syscalls, but must wait
    // until after return values are stored.
    if (eventDispatchFlag) {
        eventDispatchFlag = 0;
        Event::dispatch();
    }
}

void SvmRuntime::resetSP()
{
    setSP(SvmMemory::VIRTUAL_RAM_TOP);
}

void SvmRuntime::adjustSP(int words)
{
    setSP(SvmCpu::reg(REG_SP) + 4*words);
}

void SvmRuntime::setSP(reg_t addr)
{
    SvmCpu::setReg(REG_SP, mapSP(addr));
}

reg_t SvmRuntime::mapSP(reg_t addr)
{
    SvmMemory::PhysAddr pa;

    if (!SvmMemory::mapRAM(addr, 0, pa))
        SvmRuntime::fault(F_BAD_STACK);

    if (pa < stackLimit)
        SvmRuntime::fault(F_STACK_OVERFLOW);

    onStackModification(pa);

    return reinterpret_cast<reg_t>(pa);
}

void SvmRuntime::branch(reg_t addr)
{
    SvmCpu::setReg(REG_PC, mapBranchTarget(addr));
}

reg_t SvmRuntime::mapBranchTarget(reg_t addr)
{
    SvmMemory::PhysAddr pa;

    if (!SvmMemory::mapROCode(codeBlock, addr, pa))
        SvmRuntime::fault(F_BAD_CODE_ADDRESS);

    return reinterpret_cast<reg_t>(pa);
}

void SvmRuntime::longLDRSP(unsigned reg, unsigned offset)
{
    SvmMemory::VirtAddr va = SvmCpu::reg(REG_SP) + (offset << 2);
    SvmMemory::PhysAddr pa;

    ASSERT((va & 3) == 0);
    ASSERT(reg < 8);

    if (SvmMemory::mapRAM(va, sizeof(uint32_t), pa))
        SvmCpu::setReg(reg, *reinterpret_cast<uint32_t*>(pa));
    else
        SvmRuntime::fault(F_LONG_STACK_LOAD);
}

void SvmRuntime::longSTRSP(unsigned reg, unsigned offset)
{
    SvmMemory::VirtAddr va = SvmCpu::reg(REG_SP) + (offset << 2);
    SvmMemory::PhysAddr pa;

    ASSERT((va & 3) == 0);
    ASSERT(reg < 8);

    if (SvmMemory::mapRAM(va, sizeof(uint32_t), pa))
        *reinterpret_cast<uint32_t*>(pa) = SvmCpu::reg(reg);
    else
        SvmRuntime::fault(F_LONG_STACK_STORE);
}

void SvmRuntime::breakpoint()
{
    /*
     * We hit a breakpoint. Point the PC back to the breakpoint
     * instruction itself, not the next instruction, and
     * signal a debugger trap.
     *
     * It's important that we go directly to SvmCpu here, and not
     * use our userReg interface. We really don't want to cause a branch,
     * which can't handle non-bundle-aligned addresses.
     *
     * We need to explicitly enter the debugger's message loop here.
     */

    SvmCpu::setReg(REG_PC, SvmCpu::reg(REG_PC) - 2);
    SvmDebugger::signal(Svm::Debugger::S_TRAP);
    SvmDebugger::messageLoop();
}
