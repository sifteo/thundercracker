/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMFrameLowering.h"
#include "SVMInstrInfo.h"
#include "SVMTargetMachine.h"
#include "SVMMachineFunctionInfo.h"
#include "Support/ErrorReporter.h"
#include "llvm/Function.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetMachine.h"
using namespace llvm;

/*
 * We're telling LLVM that our stack grows upwards, since within any
 * particular (SVM-allocated) stack frame, we really really want positive
 * offsets from SP. We could do this by using setOffsetAdjustment() below,
 * but that makes argument passing a lot hairier than it needs to be!
 *
 * So, until we have a good reason to do otherwise, we'll just claim to
 * LLVM that our stack grows upwards. What it doesn't know won't hurt it?
 *
 * Also, we always want the stack to be 4-byte aligned, but in SVM we have
 * no need for 8-byte alignment. SVM doesn't support any instructions that
 * have this requirement.
 */
SVMFrameLowering::SVMFrameLowering()
    : TargetFrameLowering(TargetFrameLowering::StackGrowsUp, 4, 0, 4) {}

void SVMFrameLowering::emitPrologue(MachineFunction &MF) const
{
    MachineBasicBlock &MBB = MF.front();
    MachineBasicBlock::iterator MBBI = MBB.begin();
    MachineFrameInfo *MFI = MF.getFrameInfo();
    DebugLoc dl = MBBI != MBB.end() ? MBBI->getDebugLoc() : DebugLoc();
    const SVMInstrInfo &TII =
        *static_cast<const SVMInstrInfo*>(MF.getTarget().getInstrInfo());

    /*
     * Align both the top and bottom of the stack frame, so that our frame
     * size and frame indices are always word-aligned.
     */

    unsigned stackSize = MFI->getStackSize();
    unsigned alignMask = getStackAlignment() - 1;
    assert(alignMask >= 3);
    stackSize = (stackSize + alignMask) & ~alignMask;
    MFI->setStackSize(stackSize);
    
    /*
     * Apply a stack frame adjustment at the beginning of this function.
     * On SVM, there are three ways to do this. In order of decreasing
     * preferability:
     *
     *   1. We can get a small adjustment (0x7F words or less) for
     *      free as part of the 'call' SVC. This stack adjustment is encoded
     *      into the function address at link time, using a FNSTACK
     *      pseudo-instruction, which is used by SVMELFProgramWriter to
     *      generate the proper relocations.
     *
     *   2. Small remainders can be allocated using SPADJ. This works for
     *      up to 31 words at a time.
     *
     *   3. Larger remainders are allocated using an indirect addrop which
     *      can allocate up to 64MB of stack space :)
     *
     * The addrop in (3) clearly trumps the other two methods for dealing
     * with truly large stack frames. But because the FNSTACK adjustment is
     * so cheap, we always try to use it. This also has the nice effect of
     * making large-stacked functions clearly visible, since they'll have
     * a 0x7F in their upper address bits.
     */

    uint32_t wordsRemaining = (stackSize + 3) / 4;

    if (wordsRemaining) {
        uint32_t chunk = std::min(wordsRemaining, (uint32_t)0x7F);
        BuildMI(MBB, MBBI, dl, TII.get(SVM::FNSTACK)).addImm(chunk << 2);
        wordsRemaining -= chunk;
    }

    if (wordsRemaining)
        report_warning(MF.getFunction(), "Large stack frame, " + Twine(stackSize) + " bytes.");

    if (wordsRemaining > 0x1F) {
        assert(wordsRemaining <= 0xFFFFFF);
        uint32_t indir = 0xc3000000 | wordsRemaining;
        LLVMContext &Ctx = MF.getFunction()->getContext();
        Constant *C = ConstantInt::get(Type::getInt32Ty(Ctx),  indir);
        unsigned cpi = MF.getConstantPool()->getConstantPoolIndex(C, 4);
        BuildMI(MBB, MBBI, dl, TII.get(SVM::SPADJi)).addConstantPoolIndex(cpi);

    } else if (wordsRemaining) {
        assert(wordsRemaining <= 0x1F);
        BuildMI(MBB, MBBI, dl, TII.get(SVM::SPADJ)).addImm(wordsRemaining << 2);
    }
}

void SVMFrameLowering::emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const
{
}

bool SVMFrameLowering::hasFP(const llvm::MachineFunction&) const
{
    return false;
}
