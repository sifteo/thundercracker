/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_FRAMEINFO_H
#define SVM_FRAMEINFO_H

#include "SVM.h"
#include "llvm/Target/TargetFrameLowering.h"

namespace llvm {

class SVMFrameLowering : public TargetFrameLowering {
public:
    SVMFrameLowering()
        : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, 8, 0) {}

    void emitPrologue(MachineFunction &MF) const;
    void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const;

    virtual bool hasFP(const llvm::MachineFunction&) const { return false; }
};

} // llvm namespace

#endif
