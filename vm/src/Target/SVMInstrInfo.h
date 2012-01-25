/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVMINSTRUCTIONINFO_H
#define SVMINSTRUCTIONINFO_H

#include "llvm/Target/TargetInstrInfo.h"
#include "SVMRegisterInfo.h"

#define GET_INSTRINFO_HEADER
#include "SVMGenInstrInfo.inc"

namespace llvm {

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

private:
    const SVMRegisterInfo RI;
};

}

#endif
