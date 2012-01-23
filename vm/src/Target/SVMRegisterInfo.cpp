/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVM.h"
#include "SVMMCTargetDesc.h"
#include "SVMRegisterInfo.h"

#define GET_REGINFO_TARGET_DESC
#include "SVMGenRegisterInfo.inc"

using namespace llvm;

SVMRegisterInfo::SVMRegisterInfo(const TargetInstrInfo &tii)
    : SVMGenRegisterInfo(SVM::R7), TII(tii) {}

const unsigned int *SVMRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const
{
    static const unsigned calleeSaved[] = { 0 };
    return calleeSaved;
}

BitVector SVMRegisterInfo::getReservedRegs(const MachineFunction &MF) const
{
    BitVector res(getNumRegs());
    return res;
}

void SVMRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj, RegScavenger *RS) const
{
}

unsigned int SVMRegisterInfo::getFrameRegister(const MachineFunction &MF) const
{
    return 0;
}
