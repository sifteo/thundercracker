/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_TARGETMACHINE_H
#define SVM_TARGETMACHINE_H

#include "SVMInstrInfo.h"
#include "SVMISelLowering.h"
#include "SVMFrameLowering.h"
#include "SVMSelectionDAGInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetFrameLowering.h"

namespace llvm {

class SVMTargetMachine : public LLVMTargetMachine {
    const TargetData DataLayout;
    SVMTargetLowering TLInfo;
    SVMSelectionDAGInfo TSInfo;
    SVMInstrInfo InstrInfo;
    SVMFrameLowering FrameLowering;

public:
    SVMTargetMachine(const Target &T);

    virtual const SVMInstrInfo *getInstrInfo() const { return &InstrInfo; }
    virtual const TargetFrameLowering  *getFrameLowering() const { return &FrameLowering; }
    virtual const SVMRegisterInfo *getRegisterInfo() const { return &InstrInfo.getRegisterInfo(); }
    virtual const SVMTargetLowering* getTargetLowering() const { return &TLInfo; }
    virtual const SVMSelectionDAGInfo* getSelectionDAGInfo() const { return &TSInfo; }
    virtual const TargetData *getTargetData() const { return &DataLayout; }

    virtual bool addInstSelector(PassManagerBase &PM, CodeGenOpt::Level OptLevel);
};

}

#endif
