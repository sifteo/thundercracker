/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMFrameLowering.h"
#include "SVMInstrInfo.h"
#include "SVMMachineFunctionInfo.h"
#include "llvm/Function.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

SVMFrameLowering::SVMFrameLowering()
    : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, 8, 0, 8) {}

void SVMFrameLowering::emitPrologue(MachineFunction &MF) const
{
    MachineFrameInfo *MFI = MF.getFrameInfo();
    unsigned stackSize = MFI->getStackSize();
    unsigned alignMask = getStackAlignment() - 1;

    stackSize = (stackSize + alignMask) & ~alignMask;
    MFI->setStackSize(stackSize);

    // On SVM, the 'call' SVC includes an SP adjustment
    MFI->setOffsetAdjustment(stackSize);
}

void SVMFrameLowering::emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const
{
}

bool SVMFrameLowering::hasFP(const llvm::MachineFunction&) const
{
    return false;
}
