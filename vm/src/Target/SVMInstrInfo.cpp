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

SVMInstrInfo::SVMInstrInfo()
  : RI(*this) {}


unsigned SVMInstrInfo::isLoadFromStackSlot(const MachineInstr *MI,
                                           int &FrameIndex) const
{
    return MI->getOpcode() == SVM::LDRsp;
}

unsigned SVMInstrInfo::isStoreToStackSlot(const MachineInstr *MI,
                                          int &FrameIndex) const
{
    return MI->getOpcode() == SVM::STRsp;
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

    if (RC != SVM::GPRegRegisterClass)
        llvm_unreachable("Can't store this register to stack slot");   

    BuildMI(MBB, MBBI, DL, get(SVM::STRsp))
        .addReg(SrcReg, getKillRegState(isKill))
        .addFrameIndex(FrameIndex);
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

    if (RC != SVM::GPRegRegisterClass)
        llvm_unreachable("Can't load this register from stack slot");   

    BuildMI(MBB, MBBI, DL, get(SVM::LDRsp), DestReg)
        .addFrameIndex(FrameIndex);
}

void SVMInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                               MachineBasicBlock::iterator MBBI, DebugLoc DL,
                               unsigned DestReg, unsigned SrcReg,
                               bool KillSrc) const
{
    if (!SVM::GPRegRegClass.contains(DestReg, SrcReg))
        llvm_unreachable("Impossible reg-to-reg copy");

    BuildMI(MBB, MBBI, DL, get(SVM::MOVSr), DestReg)
          .addReg(SrcReg, getKillRegState(KillSrc));
}
