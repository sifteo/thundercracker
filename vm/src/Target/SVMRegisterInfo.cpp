/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVM.h"
#include "SVMMCTargetDesc.h"
#include "SVMRegisterInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFrameInfo.h"

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

void SVMRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                          int SPAdj, RegScavenger *RS) const
{
    assert(SPAdj == 0 && "Not allowed to modify SP");

    unsigned i = 0;
    MachineInstr &MI = *II;
    while (!MI.getOperand(i).isFI()) {
        ++i;
        assert(i < MI.getNumOperands() && "Instr doesn't have FrameIndex operand!");
    }

    int FrameIndex = MI.getOperand(i).getIndex();
    MachineFunction &MF = *MI.getParent()->getParent();
    MachineFrameInfo *MFI = MF.getFrameInfo();

    int Offset = MF.getFrameInfo()->getObjectOffset(FrameIndex);
    Offset += MFI->getOffsetAdjustment();

    assert(Offset >= 0 && Offset < 256);
    MI.getOperand(i).ChangeToImmediate(Offset);
}

unsigned int SVMRegisterInfo::getFrameRegister(const MachineFunction &MF) const
{
    return SVM::SP;
}
