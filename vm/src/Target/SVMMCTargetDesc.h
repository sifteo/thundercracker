/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_MCTARGETDESC_H
#define SVM_MCTARGETDESC_H

#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCContext.h"

namespace llvm {
    class MCSubtargetInfo;
    class Target;
    class StringRef;

    extern Target TheSVMTarget;

    MCCodeEmitter *createSVMMCCodeEmitter(const MCInstrInfo &MCII,
                                          const MCSubtargetInfo &STI,
                                          MCContext &Ctx);

    MCAsmBackend *createSVMAsmBackend(const Target &T, StringRef TT);
}

#define GET_REGINFO_ENUM
#include "SVMGenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#include "SVMGenInstrInfo.inc"

#endif
