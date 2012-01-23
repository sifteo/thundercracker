/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMMCTargetDesc.h"
#include "SVMMCAsmInfo.h"
#include "llvm/MC/MCCodeGenInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/Support/TargetRegistry.h"

#define GET_INSTRINFO_MC_DESC
#include "SVMGenInstrInfo.inc"

#define GET_REGINFO_MC_DESC
#include "SVMGenRegisterInfo.inc"

using namespace llvm;

static MCInstrInfo *createSVMMCInstrInfo()
{
    MCInstrInfo *X = new MCInstrInfo();
    InitSVMMCInstrInfo(X);
    return X;
}

static MCRegisterInfo *createSVMMCRegisterInfo(StringRef TT)
{
    MCRegisterInfo *X = new MCRegisterInfo();
    InitSVMMCRegisterInfo(X);
    return X;
}

static MCCodeGenInfo *createSVMMCCodeGenInfo(StringRef TT, Reloc::Model RM, CodeModel::Model CM)
{
    MCCodeGenInfo *X = new MCCodeGenInfo();
    X->InitMCCodeGenInfo(RM, CM);
    return X;
}

extern "C" void LLVMInitializeSVMTargetMC() {
    RegisterMCAsmInfo<SVMMCAsmInfo> X(TheSVMTarget);
    TargetRegistry::RegisterMCCodeGenInfo(TheSVMTarget, createSVMMCCodeGenInfo);
    TargetRegistry::RegisterMCInstrInfo(TheSVMTarget, createSVMMCInstrInfo);
    TargetRegistry::RegisterMCRegInfo(TheSVMTarget, createSVMMCRegisterInfo);
}
