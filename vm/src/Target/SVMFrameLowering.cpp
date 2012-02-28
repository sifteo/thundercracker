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
#include "llvm/Function.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetMachine.h"
using namespace llvm;


SVMFrameLowering::SVMFrameLowering()
    : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, 8, 0, 8) {}

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
    stackSize = (stackSize + alignMask) & ~alignMask;
    MFI->setStackSize(stackSize);
    
    /*
     * If this frame is too big, raise an error immediately.
     */

    if (stackSize > SVMTargetMachine::getMaxStackFrameBytes()) {
        Twine Name = MF.getFunction()->getName();
        report_fatal_error("Function '" + Name + "' has oversized stack frame, "
            + Twine(stackSize) + " bytes. (Max allowed size is "
            + Twine(SVMTargetMachine::getMaxStackFrameBytes()) + " bytes)");
    }

    /*
     * On SVM, the 'call' SVC includes an SP adjustment. We emit this
     * as a FNSTACK pseudo-instruction, which is used by SVMELFProgramWriter
     * to generate the proper relocations.
     */

    if (stackSize) {
        MFI->setOffsetAdjustment(stackSize);
        BuildMI(MBB, MBBI, dl, TII.get(SVM::FNSTACK)).addImm(stackSize);
    }
}

void SVMFrameLowering::emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const
{
}

bool SVMFrameLowering::hasFP(const llvm::MachineFunction&) const
{
    return false;
}
