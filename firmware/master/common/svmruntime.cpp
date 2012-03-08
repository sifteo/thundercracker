/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "svmruntime.h"
#include "elfutil.h"
#include "flashlayer.h"
#include "svm.h"
#include "svmmemory.h"
#include "svmdebug.h"
#include "event.h"

#include <sifteo/abi.h>
#include <string.h>

#ifndef PRIxPTR // for mingw
#define PRIxPTR "x"
#endif

using namespace Svm;

FlashBlockRef SvmRuntime::codeBlock;
FlashBlockRef SvmRuntime::dataBlock;
SvmMemory::PhysAddr SvmRuntime::stackLimit;
reg_t SvmRuntime::eventFrame;
bool SvmRuntime::eventDispatchFlag;


void SvmRuntime::run(uint16_t appId)
{
    // TODO: look this up via appId
    FlashRange elf(0, 0xFFFF0000);

    Elf::ProgramInfo pInfo;
    if (!pInfo.init(elf))
        return;

    // On simulation, with the built-in debugger, point SvmDebug to
    // the proper ELF binary to load debug symbols from.
    SvmDebug::setSymbolSourceELF(elf);

    // Initialize rodata segment
    SvmMemory::setFlashSegment(pInfo.rodata.data);

    // Clear RAM (including implied BSS)
    SvmMemory::erase();
    SvmCpu::init();

    // Load RWDATA into RAM
    SvmMemory::PhysAddr rwdataPA;
    if (!pInfo.rwdata.data.isEmpty() &&
        SvmMemory::mapRAM(SvmMemory::VirtAddr(pInfo.rwdata.vaddr),
            pInfo.rwdata.size, rwdataPA)) {
        FlashStream rwDataStream(pInfo.rwdata.data);
        rwDataStream.read(rwdataPA, pInfo.rwdata.size);
    }

    // Stack setup
    SvmMemory::mapRAM(pInfo.bss.vaddr + pInfo.bss.size, 0, stackLimit);
    resetSP();

    // Tail call into user code. main() does not have a CallFrame.
    enterFunction(pInfo.entry);
    SvmCpu::run();
}

void SvmRuntime::exit()
{
    ASSERT(0 && "Not yet implemented");
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

    fp->pc = reconstructCodeAddr();
    fp->fp = sFP;
    fp->r2 = sR2;
    fp->r3 = sR3;
    fp->r4 = sR4;
    fp->r5 = sR5;
    fp->r6 = sR6;
    fp->r7 = sR7;

    // This is now the current frame
    SvmCpu::setReg(REG_FP, reinterpret_cast<reg_t>(fp));

#ifdef SVM_TRACE
    LOG(("CALL: %08x, sp-%u, Saving frame %p: pc=%08x fp=%08x r2=%08x "
        "r3=%08x r4=%08x r5=%08x r6=%08x r7=%08x\n",
        (unsigned)(addr & 0xffffff), (unsigned)(addr >> 24),
        fp, fp->pc, fp->fp, fp->r2, fp->r3, fp->r4, fp->r5, fp->r6, fp->r7));
#endif

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

#ifdef SVM_TRACE
    LOG(("TAILCALL: %08x, sp-%u, Keeping frame %"PRIxPTR"\n",
        (unsigned)(addr & 0xffffff), (unsigned)(addr >> 24), fp));
#endif

    enterFunction(addr);
}

void SvmRuntime::enterFunction(reg_t addr)
{
    // Allocate stack space for this function, and enter it
    adjustSP(-(addr >> 24));
    branch(addr);
}

void SvmRuntime::ret()
{
    reg_t regFP = SvmCpu::reg(REG_FP);
    CallFrame *fp = reinterpret_cast<CallFrame*>(regFP);

    if (fp) {
        // Restore the saved frame

#ifdef SVM_TRACE
        LOG(("RET: Restoring frame %p: pc=%08x fp=%08x r2=%08x "
            "r3=%08x r4=%08x r5=%08x r6=%08x r7=%08x\n",
            fp, fp->pc, fp->fp, fp->r2, fp->r3, fp->r4, fp->r5, fp->r6, fp->r7));
#endif

        SvmCpu::setReg(REG_FP, fp->fp);
        SvmCpu::setReg(2, fp->r2);
        SvmCpu::setReg(3, fp->r3);
        SvmCpu::setReg(4, fp->r4);
        SvmCpu::setReg(5, fp->r5);
        SvmCpu::setReg(6, fp->r6);
        SvmCpu::setReg(7, fp->r7);

        setSP(reinterpret_cast<reg_t>(fp + 1));
        branch(fp->pc);

    } else {
        // No more functions on the stack. Return from main() is exit().
        exit();
    }
    
    // If we're returning from an event handler, see if we still need
    // to dispatch any other pending events.
    if (eventFrame == regFP) {
        eventFrame = 0;
        dispatchEventsOnReturn();
    }
}

void SvmRuntime::svc(uint8_t imm8)
{
    if ((imm8 & (1 << 7)) == 0) {
        svcIndirectOperation(imm8);

    } else if ((imm8 & (0x3 << 6)) == (0x2 << 6)) {
        uint8_t syscallNum = imm8 & 0x3f;
        syscall(syscallNum);

    } else if ((imm8 & (0x7 << 5)) == (0x6 << 5)) {
        uint8_t imm5 = imm8 & 0x1f;
        adjustSP(imm5);

    } else {
        uint8_t sub = (imm8 >> 3) & 0x1f;
        unsigned r = imm8 & 0x7;

        switch (sub) {
        case 0x1c:  // 0b11100
            validate(SvmCpu::reg(r));
            break;
        case 0x1d:  // 0b11101
            SvmDebug::fault(F_RESERVED_SVC);
            break;
        case 0x1e:  // 0b11110
            call(SvmCpu::reg(r));
            break;
        case 0x1f:  // 0b11110
            tailcall(SvmCpu::reg(r));
            break;
        default:
            SvmDebug::fault(F_RESERVED_SVC);
            break;
        }
    }

    if (eventDispatchFlag) {
        eventDispatchFlag = 0;
        Event::dispatch();
    }
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
    }
    else if ((literal & TailSyscallMask) == TailSyscallTest) {
        unsigned imm15 = (literal >> 16) & 0x3ff;
        syscall(imm15);
        ret();
    }
    else if ((literal & AddropMask) == AddropTest) {
        unsigned opnum = (literal >> 24) & 0x1f;
        addrOp(opnum, literal & 0xffffff);
    }
    else if ((literal & AddropFlashMask) == AddropFlashTest) {
        unsigned opnum = (literal >> 24) & 0x1f;
        addrOp(opnum, SvmMemory::VIRTUAL_FLASH_BASE + (literal & 0xffffff));
    }
    else {
        SvmDebug::fault(F_RESERVED_SVC);
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
            SvmDebug::fault(F_PRELOAD_ADDRESS);
        break;
    case 2:
        validate(address);
        break;
    case 3:
        adjustSP(address);
        break;
    case 4:
        longSTRSP((address >> 21) & 7, address & 0x1FFFFF);
        break;
    case 5:
        longLDRSP((address >> 21) & 7, address & 0x1FFFFF);
        break;
    default:
        SvmDebug::fault(F_RESERVED_ADDROP);
        break;
    }
}

void SvmRuntime::validate(reg_t address)
{
    /*
     * Map 'address' as either a flash or RAM address, and set the
     * base pointer registers r8-9 appropriately.
     */

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

    typedef uint64_t (*SvmSyscall)(reg_t p0, reg_t p1, reg_t p2, reg_t p3,
                                   reg_t p4, reg_t p5, reg_t p6, reg_t p7);

    static const SvmSyscall SyscallTable[] = {
        #include "syscall-table.def"
    };

    if (num >= sizeof SyscallTable / sizeof SyscallTable[0]) {
        SvmDebug::fault(F_BAD_SYSCALL);
        return;
    }
    SvmSyscall fn = SyscallTable[num];
    if (!fn) {
        SvmDebug::fault(F_BAD_SYSCALL);
        return;
    }

#ifdef SVM_TRACE
    LOG(("SYSCALL: enter _SYS_%d(%"PRIxPTR", %"PRIxPTR", %"PRIxPTR", %"
        PRIxPTR", %"PRIxPTR", %"PRIxPTR", %"PRIxPTR", %"PRIxPTR")\n",
        num,
        SvmCpu::reg(0), SvmCpu::reg(1),
        SvmCpu::reg(2), SvmCpu::reg(3),
        SvmCpu::reg(4), SvmCpu::reg(5),
        SvmCpu::reg(6), SvmCpu::reg(7)));
#endif

    uint64_t result = fn(SvmCpu::reg(0), SvmCpu::reg(1),
                         SvmCpu::reg(2), SvmCpu::reg(3),
                         SvmCpu::reg(4), SvmCpu::reg(5),
                         SvmCpu::reg(6), SvmCpu::reg(7));

    uint32_t result0 = result;
    uint32_t result1 = result >> 32;

#ifdef SVM_TRACE
    LOG(("SYSCALL: leave _SYS_%d() -> %x:%x\n",
        num, result1, result0));
#endif

    SvmCpu::setReg(0, result0);
    SvmCpu::setReg(1, result1);
}

void SvmRuntime::resetSP()
{
    setSP(SvmMemory::VIRTUAL_RAM_TOP);
}

void SvmRuntime::adjustSP(int words)
{
    setSP(SvmCpu::reg(SvmCpu::REG_SP) + 4*words);
}

void SvmRuntime::setSP(reg_t addr)
{
    SvmMemory::PhysAddr pa;
    
    if (!SvmMemory::mapRAM(addr, 0, pa))
        SvmDebug::fault(F_BAD_STACK);

    if (pa < stackLimit)
        SvmDebug::fault(F_STACK_OVERFLOW);

    SvmCpu::setReg(SvmCpu::REG_SP, reinterpret_cast<reg_t>(pa));
}

void SvmRuntime::branch(reg_t addr)
{
    SvmMemory::PhysAddr pa;

    if (!SvmMemory::mapROCode(codeBlock, addr, pa))
        SvmDebug::fault(F_BAD_CODE_ADDRESS);

    SvmCpu::setReg(SvmCpu::REG_PC, reinterpret_cast<reg_t>(pa));    
}

void SvmRuntime::longLDRSP(unsigned reg, unsigned offset)
{
    SvmMemory::VirtAddr va = SvmCpu::reg(SvmCpu::REG_SP) + (offset << 2);
    SvmMemory::PhysAddr pa;

    ASSERT((va & 3) == 0);
    ASSERT(reg < 8);

    if (SvmMemory::mapRAM(va, sizeof(uint32_t), pa))
        SvmCpu::setReg(reg, *reinterpret_cast<uint32_t*>(pa));
    else
        SvmDebug::fault(F_LONG_STACK_LOAD);
}

void SvmRuntime::longSTRSP(unsigned reg, unsigned offset)
{
    SvmMemory::VirtAddr va = SvmCpu::reg(SvmCpu::REG_SP) + (offset << 2);
    SvmMemory::PhysAddr pa;

    ASSERT((va & 3) == 0);
    ASSERT(reg < 8);

    if (SvmMemory::mapRAM(va, sizeof(uint32_t), pa))
        *reinterpret_cast<uint32_t*>(pa) = SvmCpu::reg(reg);
    else
        SvmDebug::fault(F_LONG_STACK_STORE);
}
