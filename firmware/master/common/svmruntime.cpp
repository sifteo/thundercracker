/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "svmruntime.h"
#include "elfutil.h"
#include "flashlayer.h"
#include "svm.h"
#include "svmmemory.h"
#include "svmdebugpipe.h"
#include "svmdebugger.h"
#include "event.h"
#include "tasks.h"
#include "radio.h"

#include <sifteo/abi.h>
#include <string.h>
#include <stdlib.h>

using namespace Svm;

FlashBlockRef SvmRuntime::codeBlock;
FlashBlockRef SvmRuntime::dataBlock;
SvmMemory::PhysAddr SvmRuntime::stackLimit;
reg_t SvmRuntime::eventFrame;
bool SvmRuntime::eventDispatchFlag;


static void logTitleInfo(Elf::ProgramInfo &pInfo)
{
#ifdef SIFTEO_SIMULATOR
    FlashBlockRef ref;

    const char *title = pInfo.meta.getString(ref, _SYS_METADATA_TITLE_STR);
    LOG(("SVM: Preparing to run title \"%s\"\n", title ? title : "(untitled)"));

    const _SYSUUID *uuid = pInfo.meta.getValue<_SYSUUID>(ref, _SYS_METADATA_UUID);
    if (uuid) {
        LOG(("SVM: Title UUID is {"));
        for (unsigned i = 0; i < 16; i++)
            LOG(("%02x%s", uuid->bytes[i], (i == 3 || i == 5 || i == 7 || i == 9) ? "-" : ""));
        LOG(("}\n"));
    }
#endif
}

static void xxxBootstrapAssets(Elf::ProgramInfo &pInfo)
{
    /*
     * XXX: BIG HACK.
     *
     * Temporary code to load the bootstrap assets specified
     * in a game's metadata. Normally this would be handled by
     * the system menu.
     */

    // XXX: Also a hack.. installing on cube 0 only.
    _SYSCubeIDVector cubes = Intrinsic::LZ(0);

    FlashBlockRef ref;
    uint32_t actualSize;
    _SYSMetadataBootAsset *vec = (_SYSMetadataBootAsset*)
        pInfo.meta.get(ref, _SYS_METADATA_BOOT_ASSET, sizeof *vec, actualSize);
    if (!vec) return;
    unsigned count = actualSize / sizeof *vec;

    // Temporarily enable the cubes we'll be using.
    _SYS_enableCubes(cubes);

    for (unsigned i = 0; i < count; i++) {
        _SYSMetadataBootAsset &BA = vec[i];

        // Allocate some things in user RAM.
        const SvmMemory::VirtAddr loaderVA = 0x10000;
        const SvmMemory::VirtAddr groupVA = 0x11000;

        SvmMemory::PhysAddr loaderPA;
        SvmMemory::PhysAddr groupPA;
        SvmMemory::mapRAM(loaderVA, 1, loaderPA);
        SvmMemory::mapRAM(groupVA, 1, groupPA);

        _SYSAssetLoader *loader = reinterpret_cast<_SYSAssetLoader*>(loaderPA);
        _SYSAssetGroup *group = reinterpret_cast<_SYSAssetGroup*>(groupPA);

        loader->cubeVec = 0;
        group->pHdr = BA.pHdr;

        if (_SYS_asset_findInCache(group)) {
            LOG(("SVM: Bootstrap asset group %s already installed\n",
                SvmDebugPipe::formatAddress(BA.pHdr).c_str()));
            continue;
        }

        LOG(("SVM: Installing bootstrap asset group %s in slot %d\n",
            SvmDebugPipe::formatAddress(BA.pHdr).c_str(), BA.slot));

        if (!_SYS_asset_loadStart(loader, group, BA.slot, cubes)) {
            // Out of space. Erase the slot first.
            LOG(("SVM: Erasing asset slot\n"));
            _SYS_asset_slotErase(BA.slot);
            _SYS_asset_loadStart(loader, group, BA.slot, cubes);
        }

        while ((loader->complete & cubes) != cubes) {
            Tasks::work();
            Radio::halt();
        }

        _SYS_asset_loadFinish(loader);

        LOG(("SVM: Finished instaling bootstrap asset group %s\n",
            SvmDebugPipe::formatAddress(BA.pHdr).c_str()));
    }

    _SYS_disableCubes(cubes);
}

void SvmRuntime::run(uint16_t appId)
{
    // TODO: look this up via appId
    FlashRange elf(0, 0xFFFF0000);

    Elf::ProgramInfo pInfo;
    if (!pInfo.init(elf))
        return;

    // On simulator builds, log some info about the program we're running
    logTitleInfo(pInfo);

    // On simulation, with the built-in debugger, point SvmDebug to
    // the proper ELF binary to load debug symbols from.
    SvmDebugPipe::setSymbolSourceELF(elf);

    // Initialize rodata segment
    SvmMemory::setFlashSegment(pInfo.rodata.data);

    // Asset setup
    xxxBootstrapAssets(pInfo);

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

    reg_t sp = mapSP(SvmMemory::VIRTUAL_RAM_TOP);   // reset sp
    int spAdjust = (pInfo.entry >> 24) * 4;         // Allocate stack space for this function

    SvmCpu::run(mapSP(sp - spAdjust), mapBranchTarget(pInfo.entry));
}

void SvmRuntime::exit()
{
    // XXX - Temporary

#ifdef SIFTEO_SIMULATOR
    ::exit(0);
#endif
    while (1);
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

    // Unhandled fault; exit
    exit();
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
            (unsigned)(addr & 0xffffff), (unsigned)(addr >> 24),
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
            (unsigned)(addr & 0xffffff), (unsigned)(addr >> 24),
            reinterpret_cast<void*>(fp)));
    });

    enterFunction(addr);
}

void SvmRuntime::enterFunction(reg_t addr)
{
    // Allocate stack space for this function, and enter it
    adjustSP(-(addr >> 24));
    branch(addr);
}

void SvmRuntime::ret(unsigned actions)
{
    reg_t regFP = SvmCpu::reg(REG_FP);
    CallFrame *fp = reinterpret_cast<CallFrame*>(regFP);

    if (!fp) {
        // No more functions on the stack. Return from main() is exit().
        if (actions & RET_EXIT)
            exit();
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

void SvmRuntime::svc(uint8_t imm8)
{
    if ((imm8 & (1 << 7)) == 0) {
        if (imm8 == 0)
            ret();
        else
            svcIndirectOperation(imm8);

    } else if ((imm8 & (0x3 << 6)) == (0x2 << 6)) {
        uint8_t syscallNum = imm8 & 0x3f;
        syscall(syscallNum);

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
        tailsyscall(imm15);
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

    // Poll for pending userspace tasks on our way up. This is akin to a
    // deferred procedure call (DPC) in Win32.
    Tasks::work();
}

void SvmRuntime::tailsyscall(unsigned num)
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
