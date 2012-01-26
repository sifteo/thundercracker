/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMMCTargetDesc.h"
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
using namespace llvm;

namespace {
    
class SVMELFObjectWriter : public MCELFObjectTargetWriter {
public:
    SVMELFObjectWriter(bool is64Bit, Triple::OSType OSType, uint16_t EMachine,
        bool HasRelocationAddend)
        : MCELFObjectTargetWriter(is64Bit, OSType, EMachine,
            HasRelocationAddend) {}
};

class SVMAsmBackend : public MCAsmBackend {
public:    
    SVMAsmBackend(const Target &T, Triple::OSType O)
        : MCAsmBackend(), OSType(O) {}

    Triple::OSType OSType;

    MCELFObjectTargetWriter *createELFObjectTargetWriter() const
    {
        return new SVMELFObjectWriter(false, OSType, ELF::EM_ARM, false);
    }

    MCObjectWriter *createObjectWriter(raw_ostream &OS) const
    {
        return createELFObjectWriter(createELFObjectTargetWriter(), OS, true);
    }

    unsigned getNumFixupKinds() const
    {
        return 1;
    }

    void ApplyFixup(const MCFixup &Fixup, char *Data, unsigned DataSize,
                    uint64_t Value) const
    {
    }

    bool MayNeedRelaxation(const MCInst &Inst) const
    {
        return false;
    }

    void RelaxInstruction(const MCInst &Inst, MCInst &Res) const
    {
    }

    bool WriteNopData(uint64_t Count, MCObjectWriter *OW) const
    {
        assert((Count % 2) == 0);
        for (unsigned i = 0, c = Count/2; i < c; i++)
            OW->Write16(0);
        return true;
    }
};

}  // end namespace

MCAsmBackend *llvm::createSVMAsmBackend(const Target &T, StringRef TT)
{
    Triple TheTriple(TT);
    return new SVMAsmBackend(T, Triple(TT).getOS());
}
