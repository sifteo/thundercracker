/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMInstrInfo.h"
#include "SVMMCTargetDesc.h"
#include "SVM.h"
#include "SVMMachineFunctionInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"

#define GET_INSTRINFO_CTOR
#include "SVMGenInstrInfo.inc"

using namespace llvm;

SVMCC::CondCodes SVMCC::mapTo(ISD::CondCode CC)
{
    switch (CC) {
    default: llvm_unreachable("Unknown condition code!");
    case ISD::SETNE:  return SVMCC::NE;
    case ISD::SETEQ:  return SVMCC::EQ;
    case ISD::SETGT:  return SVMCC::GT;
    case ISD::SETGE:  return SVMCC::GE;
    case ISD::SETLT:  return SVMCC::LT;
    case ISD::SETLE:  return SVMCC::LE;
    case ISD::SETUGT: return SVMCC::HI;
    case ISD::SETUGE: return SVMCC::HS;
    case ISD::SETULT: return SVMCC::LO;
    case ISD::SETULE: return SVMCC::LS;
    }
}

SVMInstrInfo::SVMInstrInfo()
  : RI(*this) {}

unsigned SVMInstrInfo::isLoadFromStackSlot(const MachineInstr *MI,
                                           int &FrameIndex) const
{
    switch (MI->getOpcode()) {
        
    case SVM::LDRsp:
        FrameIndex = MI->getOperand(1).getIndex();
        return true;

    }
    return false;
}

unsigned SVMInstrInfo::isStoreToStackSlot(const MachineInstr *MI,
                                          int &FrameIndex) const
{
    switch (MI->getOpcode()) {
        
    case SVM::STRsp:
        FrameIndex = MI->getOperand(1).getIndex();
        return true;

    }
    return false;
}

void SVMInstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
                                       MachineBasicBlock::iterator MBBI,
                                       unsigned SrcReg, bool isKill, int FrameIndex,
                                       const TargetRegisterClass *RC,
                                       const TargetRegisterInfo *TRI) const
{
    DebugLoc DL;
    if (MBBI != MBB.end())
        DL = MBBI->getDebugLoc();

    if (RC == SVM::GPRegRegisterClass)
        BuildMI(MBB, MBBI, DL, get(SVM::STRsp))
            .addReg(SrcReg, getKillRegState(isKill))
            .addFrameIndex(FrameIndex)      // SelectAddrSP - Base
            .addImm(0);                     // SelectAddrSP - Offset

    else
        llvm_unreachable("Can't store this register to stack slot");   
}

void SVMInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                        MachineBasicBlock::iterator MBBI,
                                        unsigned DestReg, int FrameIndex,
                                        const TargetRegisterClass *RC,
                                        const TargetRegisterInfo *TRI) const
{
    DebugLoc DL;
    if (MBBI != MBB.end())
        DL = MBBI->getDebugLoc();

    if (RC == SVM::GPRegRegisterClass)
        BuildMI(MBB, MBBI, DL, get(SVM::LDRsp), DestReg)
            .addFrameIndex(FrameIndex)      // SelectAddrSP - Base
            .addImm(0);                     // SelectAddrSP - Offset

    else
        llvm_unreachable("Can't load this register from stack slot");   
}

void SVMInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                               MachineBasicBlock::iterator MBBI, DebugLoc DL,
                               unsigned DestReg, unsigned SrcReg,
                               bool KillSrc) const
{
    if (SVM::GPRegRegClass.contains(DestReg, SrcReg))
        // GPR <- GPR
        BuildMI(MBB, MBBI, DL, get(SVM::MOVSr), DestReg)
            .addReg(SrcReg, getKillRegState(KillSrc));

    else if (SVM::GPRegRegClass.contains(SrcReg) && SVM::BPRegRegClass.contains(DestReg))
        // BP <- GPR
        BuildMI(MBB, MBBI, DL, get(SVM::PTR), DestReg)
            .addReg(SrcReg, getKillRegState(KillSrc));

    else
        llvm_unreachable("Can't copy special-purpose register");
}

bool SVMInstrInfo::AnalyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                                 MachineBasicBlock *&FBB,
                                 SmallVectorImpl<MachineOperand> &Cond,
                                 bool AllowModify) const
{
    /*
     * Find the last instruction in the block. If there is no terminator,
     * it implicitly falls through to the next block.
     */
    
    MachineBasicBlock::iterator I = MBB.end();
    if (I == MBB.begin()) 
        return false;
    --I;

    // Skip debug values
    while (I->isDebugValue()) {
        if (I == MBB.begin())
            return false;
        --I;
    }
    
    if (!isUnpredicatedTerminator(I))
        return false;
    
    // Actually the last instruction
    MachineInstr *lastI = I;
    unsigned lastOp = lastI->getOpcode();
    
    if (I == MBB.begin() || !isUnpredicatedTerminator(--I)) {
        // Only a single terminator instruction
        
        if (isUncondBranchOpcode(lastOp)) {
            // Unconditional branch
            TBB = lastI->getOperand(0).getMBB();
            return false;
        }
    }
        
    // Unhandled
    return true;
}

unsigned SVMInstrInfo::RemoveBranch(MachineBasicBlock &MBB) const
{
    unsigned removedInstructions = 0;
    MachineBasicBlock::iterator I = MBB.end();
    if (I == MBB.begin()) 
        return false;
    --I;

    do {
        if (I->isDebugValue()) {
            --I;
        } else if (isBranchOpcode(I->getOpcode())) {
            I->eraseFromParent();
            removedInstructions++;
            I = MBB.end();
        } else {
            break;
        }
    } while (I != MBB.begin());

    return removedInstructions;
}

unsigned SVMInstrInfo::InsertBranch(MachineBasicBlock &MBB,
    MachineBasicBlock *TBB, MachineBasicBlock *FBB,
    const SmallVectorImpl<MachineOperand> &Cond, DebugLoc DL) const
{
    assert(FBB == NULL && "Conditional branch analysis not yet implemented");
    
    BuildMI(&MBB, DL, get(SVM::B)).addMBB(TBB);
    return 1;
}
