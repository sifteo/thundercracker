/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo VM (SVM) Target for LLVM
 *
 * Micah Elizabeth Scott <micah@misc.name>
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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

    static bool inline isUncondNearBranchOpcode(unsigned op) {
        return op == SVM::B;
    }

    static bool inline isCondNearBranchOpcode(unsigned op) {
        return op == SVM::Bcc;
    }

    static bool inline isNearBranchOpcode(unsigned op) {
        return isCondNearBranchOpcode(op) || isUncondNearBranchOpcode(op);
    }

private:
    const SVMRegisterInfo RI;
};

}

#endif
