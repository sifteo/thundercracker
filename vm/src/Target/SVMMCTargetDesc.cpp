/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMMCTargetDesc.h"
#include "SVMMCAsmInfo.h"
#include "SVMInstPrinter.h"
#include "llvm/MC/MCCodeGenInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
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
    InitSVMMCRegisterInfo(X, SVM::R7);
    return X;
}

static MCCodeGenInfo *createSVMMCCodeGenInfo(StringRef TT, Reloc::Model RM, CodeModel::Model CM)
{
    MCCodeGenInfo *X = new MCCodeGenInfo();
    X->InitMCCodeGenInfo(RM, CM);
    return X;
}

static MCStreamer *createMCStreamer(const Target &T, StringRef TT,
                                    MCContext &Ctx, MCAsmBackend &MAB,
                                    raw_ostream &_OS,
                                    MCCodeEmitter *_Emitter,
                                    bool RelaxAll,
                                    bool NoExecStack)
{
  Triple TheTriple(TT);
  return createELFStreamer(Ctx, MAB, _OS, _Emitter, RelaxAll, NoExecStack);
}

static MCInstPrinter *createSVMMCInstPrinter(const Target &T, unsigned SyntaxVariant,
                                             const MCAsmInfo &MAI, const MCSubtargetInfo &STI)
{
    return new SVMInstPrinter(MAI);
}

extern "C" void LLVMInitializeSVMTargetMC() {
    RegisterMCAsmInfo<SVMMCAsmInfo> X(TheSVMTarget);
    TargetRegistry::RegisterMCCodeGenInfo(TheSVMTarget, createSVMMCCodeGenInfo);
    TargetRegistry::RegisterMCInstrInfo(TheSVMTarget, createSVMMCInstrInfo);
    TargetRegistry::RegisterMCRegInfo(TheSVMTarget, createSVMMCRegisterInfo);
    TargetRegistry::RegisterMCCodeEmitter(TheSVMTarget, createSVMMCCodeEmitter);
    TargetRegistry::RegisterMCAsmBackend(TheSVMTarget, createSVMAsmBackend);
    TargetRegistry::RegisterMCObjectStreamer(TheSVMTarget, createMCStreamer);
    TargetRegistry::RegisterMCInstPrinter(TheSVMTarget, createSVMMCInstPrinter);
}
