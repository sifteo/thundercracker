/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVMINSTRUCTIONINFO_H
#define SVMINSTRUCTIONINFO_H

#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "SVMRegisterInfo.h"
#include "SVMMCTargetDesc.h"

#define GET_INSTRINFO_HEADER
#include "SVMGenInstrInfo.inc"

namespace llvm {
    
// Condition codes
namespace SVMCC {
    enum CondCodes {
        EQ, NE, HS, LO, MI, PL, VS, VC, HI, LS, GE, LT, GT, LE, AL
    };
    CondCodes mapTo(ISD::CondCode CC);
}

// Target operand flags
namespace SVMTOF {
    enum TFlags {
        NONE = 0,
        CALL,
        TAIL_CALL,
    };
}

class SVMInstrInfo : public SVMGenInstrInfo {
public:
    explicit SVMInstrInfo();
    virtual const SVMRegisterInfo &getRegisterInfo() const { return RI; }

    virtual unsigned isLoadFromStackSlot(const MachineInstr *MI,
                                         int &FrameIndex) const;

    virtual unsigned isStoreToStackSlot(const MachineInstr *MI,
                                        int &FrameIndex) const;

    virtual void storeRegToStackSlot(MachineBasicBlock &MBB,
                                     MachineBasicBlock::iterator MBBI,
                                     unsigned SrcReg, bool isKill, int FrameIndex,
                                     const TargetRegisterClass *RC,
                                     const TargetRegisterInfo *TRI) const;

    virtual void loadRegFromStackSlot(MachineBasicBlock &MBB,
                                      MachineBasicBlock::iterator MBBI,
                                      unsigned DestReg, int FrameIndex,
                                      const TargetRegisterClass *RC,
                                      const TargetRegisterInfo *TRI) const;

    virtual void copyPhysReg(MachineBasicBlock &MBB,
                             MachineBasicBlock::iterator MBBI, DebugLoc DL,
                             unsigned DestReg, unsigned SrcReg,
                             bool KillSrc) const;

    virtual bool AnalyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                               MachineBasicBlock *&FBB,
                               SmallVectorImpl<MachineOperand> &Cond,
                               bool AllowModify) const;

    virtual unsigned RemoveBranch(MachineBasicBlock &MBB) const;

    virtual unsigned InsertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
                                  MachineBasicBlock *FBB,
                                  const SmallVectorImpl<MachineOperand> &Cond,
                                  DebugLoc DL) const;                        

private:
    const SVMRegisterInfo RI;

    bool inline isUncondBranchOpcode(unsigned op) const {
        return op == SVM::B;
    }

    bool inline isCondBranchOpcode(unsigned op) const {
        return op == SVM::Bcc;
    }

    bool inline isBranchOpcode(unsigned op) const {
        return isCondBranchOpcode(op) || isUncondBranchOpcode(op);
    }
};

}

#endif
