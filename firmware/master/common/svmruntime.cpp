/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "svmruntime.h"
#include "elfutil.h"
#include "flashlayer.h"
#include "svm.h"
#include "svmmemory.h"

#include <sifteo/abi.h>
#include <string.h>

using namespace Svm;

FlashBlockRef SvmRuntime::codeBlock;
FlashBlockRef SvmRuntime::dataBlock;
SvmMemory::PhysAddr SvmRuntime::stackLimit;


void SvmRuntime::run(uint16_t appId)
{
    // TODO: look this up via appId
    FlashRange elf(0, 0xFFFF0000);

    Elf::ProgramInfo pInfo;
    if (!pInfo.init(elf))
        return;

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
    setSP(SvmMemory::VIRTUAL_RAM_TOP);

    // Tail call into user code
    call(pInfo.entry);
    SvmCpu::run();
}

void SvmRuntime::fault(FaultCode code)
{
    // TODO: implement

    LOG(("***\n"
         "*** VM FAULT code %d\n"
         "***\n"
         "***   PC: %08x SP: %"PRIxPTR"\n"
         "***  GPR: %08x %08x %08x %08x\n"
         "***       %08x %08x %08x %08x\n"
         "***\n",
         code, reconstructCodeAddr(),
         SvmCpu::reg(REG_SP),
         (unsigned) SvmCpu::reg(0), (unsigned) SvmCpu::reg(1),
         (unsigned) SvmCpu::reg(2), (unsigned) SvmCpu::reg(3),
         (unsigned) SvmCpu::reg(4), (unsigned) SvmCpu::reg(5),
         (unsigned) SvmCpu::reg(6), (unsigned) SvmCpu::reg(7)));

    ASSERT(0);
}

void SvmRuntime::exit()
{
    ASSERT(0 && "Not yet implemented");
}

void SvmRuntime::call(reg_t addr)
{
#if 0 // TODO: Implement call/return
    StackFrame frame = {
        SvmCpu::reg(7),
        SvmCpu::reg(6),
        SvmCpu::reg(5),
        SvmCpu::reg(4),
        SvmCpu::reg(3),
        SvmCpu::reg(2),
        SvmCpu::reg(SvmCpu::REG_FP),
        SvmCpu::reg(SvmCpu::REG_SP)
    };

    reg_t sp = SvmCpu::reg(SvmCpu::REG_SP);
    const intptr_t framesize = sizeof(frame);
    reg_t *stk = reinterpret_cast<reg_t*>(sp) - framesize;
    memcpy(stk, &frame, framesize);

    adjustSP(framesize);

    SvmCpu::setReg(SvmCpu::REG_LR, SvmCpu::reg(SvmCpu::REG_PC));
#endif

    adjustSP(-(addr >> 24));
    branch(addr);
}

void SvmRuntime::svc(uint8_t imm8)
{
    if ((imm8 & (1 << 7)) == 0) {
        svcIndirectOperation(imm8);
        return;
    }
    if ((imm8 & (0x3 << 6)) == (0x2 << 6)) {
        uint8_t syscallNum = imm8 & 0x3f;
        syscall(syscallNum);
        return;
    }
    if ((imm8 & (0x7 << 5)) == (0x6 << 5)) {
        uint8_t imm5 = imm8 & 0x1f;
        adjustSP(imm5);
        return;
    }

    uint8_t sub = (imm8 >> 3) & 0x1f;
    unsigned r = imm8 & 0x7;

    switch (sub) {
    case 0x1c:  { // 0b11100
        validate(SvmCpu::reg(r));
        return;
    }
    case 0x1d:  // 0b11101
        fault(F_RESERVED_SVC);
        return;
    case 0x1e: { // 0b11110
        call(SvmCpu::reg(r));
        return;
    }
    case 0x1f:  // 0b11110
        call(SvmCpu::reg(r));
        _SYS_ret();
        return;
    default:
        fault(F_RESERVED_SVC);
        break;
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
        call(literal);
        _SYS_ret();
    }
    else if ((literal & IndirectSyscallMask) == IndirectSyscallTest) {
        unsigned imm15 = (literal >> 16) & 0x3ff;
        syscall(imm15);
    }
    else if ((literal & TailSyscallMask) == TailSyscallTest) {
        unsigned imm15 = (literal >> 16) & 0x3ff;
        syscall(imm15);
        _SYS_ret();
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
        fault(F_RESERVED_SVC);
    }
}

void SvmRuntime::addrOp(uint8_t opnum, reg_t address)
{
    switch (opnum) {
    case 0:
        branch(address);
        break;
    case 2:
        validate(address);
        break;
    default:
        fault(F_RESERVED_ADDROP);
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
        fault(F_BAD_SYSCALL);
        return;
    }
    SvmSyscall fn = SyscallTable[num];
    if (!fn) {
        fault(F_BAD_SYSCALL);
        return;
    }

#ifdef SVM_TRACE
    LOG(("SYSCALL: enter _SYS_%d(%x, %x, %x, %x, %x, %x, %x, %x)\n",
        num,
        (unsigned)SvmCpu::reg(0), (unsigned)SvmCpu::reg(1),
        (unsigned)SvmCpu::reg(2), (unsigned)SvmCpu::reg(3),
        (unsigned)SvmCpu::reg(4), (unsigned)SvmCpu::reg(5),
        (unsigned)SvmCpu::reg(6), (unsigned)SvmCpu::reg(7)));
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

void SvmRuntime::adjustSP(int words)
{
    setSP(SvmCpu::reg(SvmCpu::REG_SP) + 4*words);
}

void SvmRuntime::setSP(reg_t addr)
{
    SvmMemory::PhysAddr pa;
    
    if (!SvmMemory::mapRAM(addr, 0, pa))
        fault(F_BAD_STACK);

    if (pa < stackLimit)
        fault(F_STACK_OVERFLOW);

    SvmCpu::setReg(SvmCpu::REG_SP, reinterpret_cast<reg_t>(pa));
}

void SvmRuntime::branch(reg_t addr)
{
    SvmMemory::PhysAddr pa;

    if (!SvmMemory::mapROCode(codeBlock, addr, pa))
        fault(F_BAD_CODE_ADDRESS);

    SvmCpu::setReg(SvmCpu::REG_PC, reinterpret_cast<reg_t>(pa));    
}
