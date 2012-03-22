/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVM.h"
#include "SVMMCTargetDesc.h"
#include "SVMRegisterInfo.h"
#include "llvm/Constants.h"
#include "llvm/Type.h"
#include "llvm/Function.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineConstantPool.h"

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
    assert(MFI->getOffsetAdjustment() == 0);

    if (Offset < 0) {
        /*
         * Negative offsets are special; they represent offsets into the
         * caller's stack frame. We can calculate that offset now that we
         * know the final stack frame size of the current function.
         *
         * We need to reach upward, past this function's stack frame, and
         * past the SVM CallFrame, into the bottom of the caller's frame.
         */

        const int CallFrameSize = 8 * sizeof(uint32_t);

        Offset = -Offset - 1;
        Offset += MFI->getStackSize();
        Offset += CallFrameSize;
    }

    // Extract offsets from instructions that have immediate offsets too
    switch (MI.getOpcode()) {
    case SVM::LDRsp:
    case SVM::STRsp:
        Offset += MI.getOperand(i+1).getImm();
        MI.getOperand(i+1).ChangeToImmediate(0);
        break;
    }

    int Remaining = Offset;

    if (MI.isDebugValue()) {
        MI.getOperand(i).ChangeToImmediate(Offset);
        return;
    }

    // Encode as much offset as possible into the original instruction.
    switch (MI.getOpcode()) {

        // Encodings with an 8-bit unsigned word offset.
        // Supports multiples of four, from 0 to 1020.
    case SVM::LDRsp:
    case SVM::STRsp:
    case SVM::ADDsp: {
        unsigned Imm = std::min(Remaining & ~3, 1020);
        MI.getOperand(i).ChangeToImmediate(Imm);
        Remaining -= Imm;
        break;
    }
    
    default:
        assert(0 && "Unsupported instruction in eliminateFrameIndex()");
    }

    // If there's a residual, rewrite the instruction further.
    if (Remaining == 0)
        return;

    switch (MI.getOpcode()) {

        // ADDsp: Put in additional ADD instructions afterward, to cover
        // the residual offset. Doesn't require any additional registers.
    case SVM::ADDsp:
        do {
            unsigned Imm = std::min(Remaining, 0xFF);
            II++;
            BuildMI(MBB, II, dl, TII.get(SVM::ADDSi8))
                .addReg(MI.getOperand(0).getReg())
                .addReg(MI.getOperand(0).getReg())
                .addImm(Imm);
            II--;
            Remaining -= Imm;
        } while (Remaining != 0);
        break;

        // Large stack load/stores -> addrops. For correctness, we need
        // to be able to do spills to large stack frames without corrupting
        // state like r8-r9 or using any additional registers. So, we use
        // this slow addrop to do all the work.
    case SVM::STRsp:
        rewriteLongStackOp(II, 0xc4000000, Offset, SVM::STRspi);
        break;
    case SVM::LDRsp:
        rewriteLongStackOp(II, 0xc5000000, Offset, SVM::LDRspi);
        break;

    default:
        assert(0 && "Unsupported SP offset in eliminateFrameIndex()");
    }
}

unsigned int SVMRegisterInfo::getFrameRegister(const MachineFunction &MF) const
{
    return SVM::SP;
}

void SVMRegisterInfo::rewriteLongStackOp(MachineBasicBlock::iterator &II,
    uint32_t Op, int Offset, unsigned Instr) const
{
    /*
     * Replace a native stack load/store at *II with a long stack load/store
     * SVC. This rewrite needs to allow spilling to and from large stack frames
     * without corrupting any other machine state.
     */

    MachineInstr &MI = *II;
    DebugLoc dl = MI.getDebugLoc();
    MachineBasicBlock &MBB = *MI.getParent();
    MachineFunction &MF = *MI.getParent()->getParent();
    LLVMContext &Ctx = MF.getFunction()->getContext();

    assert((Offset & 3) == 0);
    assert(Offset <= 0x1FFFFF);
    Op |= Offset >> 2;

    unsigned reg = MI.getOperand(0).getReg();
    assert(reg >= SVM::R0 && reg <= SVM::R7);
    Op |= (reg - SVM::R0) << 21;

    Constant *C = ConstantInt::get(Type::getInt32Ty(Ctx), Op);
    unsigned cpi = MF.getConstantPool()->getConstantPoolIndex(C, 4);

    II++;
    BuildMI(MBB, II, dl, TII.get(Instr)).addReg(reg).addConstantPoolIndex(cpi);
    II--;

    MI.eraseFromParent();
}

