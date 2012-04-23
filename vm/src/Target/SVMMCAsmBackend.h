/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_MCASMBACKEND_H
#define SVM_MCASMBACKEND_H

#include "SVM.h"
#include "SVMMCTargetDesc.h"
#include "SVMFixupKinds.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCDirectives.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ELF.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Triple.h"

namespace llvm {

class SVMAsmBackend : public MCAsmBackend {
public:    
    SVMAsmBackend(const Target &T, Triple::OSType O)
        : MCAsmBackend(), OSType(O) {}

    Triple::OSType OSType;

    MCObjectWriter *createObjectWriter(raw_ostream &OS) const;
    unsigned getNumFixupKinds() const;
    
    const MCFixupKindInfo &getFixupKindInfo(MCFixupKind Kind) const;
    static const MCFixupKindInfo &getStaticFixupKindInfo(MCFixupKind Kind);

    void ApplyFixup(const MCFixup &Fixup, char *Data, unsigned DataSize,
        uint64_t Value) const;
    static void ApplyStaticFixup(MCFixupKind Kind, char *Data, int32_t Value);

    bool MayNeedRelaxation(const MCInst &Inst) const;
    void RelaxInstruction(const MCInst &Inst, MCInst &Res) const;

    bool WriteNopData(uint64_t Count, MCObjectWriter *OW) const;
};

}  // end namespace

#endif
