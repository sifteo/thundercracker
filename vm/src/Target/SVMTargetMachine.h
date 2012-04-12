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
#include "SVMSubtarget.h"
#include "llvm/ADT/StringRef.h"
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
    SVMSubtarget Subtarget;

public:
    SVMTargetMachine(const Target &T, StringRef TT, 
                     StringRef CPU, StringRef FS,
                     Reloc::Model RM, CodeModel::Model CM);

    static uint32_t getBlockSize();
    static uint32_t getBundleSize();
    static uint32_t getFlashBase();
    static uint32_t getRAMBase();
    static uint8_t getPaddingByte();
    static const char *getDataLayoutString();
    static bool isTargetCompatible(LLVMContext& Context, const TargetData &TD);

    virtual const SVMInstrInfo *getInstrInfo() const { return &InstrInfo; }
    virtual const TargetFrameLowering  *getFrameLowering() const { return &FrameLowering; }
    virtual const SVMRegisterInfo *getRegisterInfo() const { return &InstrInfo.getRegisterInfo(); }
    virtual const SVMTargetLowering* getTargetLowering() const { return &TLInfo; }
    virtual const SVMSelectionDAGInfo* getSelectionDAGInfo() const { return &TSInfo; }
    virtual const TargetData *getTargetData() const { return &DataLayout; }
    virtual const SVMSubtarget *getSubtargetImpl() const { return &Subtarget; }

    virtual bool addInstSelector(PassManagerBase &PM, CodeGenOpt::Level OptLevel);
    virtual bool addPreEmitPass(PassManagerBase &PM, CodeGenOpt::Level OptLevel);
};

}

#endif
