/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVM.h"
#include "SVMMCTargetDesc.h"
#include "SVMRegisterInfo.h"
#include "llvm/Target/TargetInstrInfo.h"
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
    DebugLoc dl = MI.getDebugLoc();
    MachineBasicBlock &MBB = *MI.getParent();
    MachineFunction &MF = *MI.getParent()->getParent();
    MachineFrameInfo *MFI = MF.getFrameInfo();

    int Offset = MF.getFrameInfo()->getObjectOffset(FrameIndex);
    Offset += MFI->getOffsetAdjustment();

    // Encode as much offset as possible into the original instruction.
    switch (MI.getOpcode()) {

        // Encodings with an 8-bit unsigned word offset.
        // Supports multiples of four, from 0 to 1020.
    case SVM::LDRsp:
    case SVM::STRsp:
    case SVM::ADDsp: {
        unsigned Imm = std::min(Offset & ~3, 1020);
        MI.getOperand(i).ChangeToImmediate(Imm);
        Offset -= Imm;
        break;
    }
    
    default:
        assert(0 && "Unsupported instruction in eliminateFrameIndex()");
    }

    // If there's a residual, rewrite the instruction further.
    if (Offset == 0)
        return;

    switch (MI.getOpcode()) {

        // ADDsp: Put in additional ADD instructions afterward, to cover
        // the residual offset. Doesn't require any additional registers.
    case SVM::ADDsp:
        do {
            unsigned Imm = std::min(Offset, 255);
            II++;
            BuildMI(MBB, II, dl, TII.get(SVM::ADDSi8))
                .addReg(MI.getOperand(0).getReg())
                .addReg(MI.getOperand(0).getReg())
                .addImm(Imm);
            II--;
            Offset -= Imm;
        } while (Offset != 0);
        break;

    default:
        assert(0 && "Unsupported SP offset in eliminateFrameIndex()");
    }

    assert(Offset == 0);
}

unsigned int SVMRegisterInfo::getFrameRegister(const MachineFunction &MF) const
{
    return SVM::SP;
}
