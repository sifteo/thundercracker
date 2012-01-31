/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

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
        return SVM::NumTargetFixupKinds;
    }
    
    const MCFixupKindInfo &getFixupKindInfo(MCFixupKind Kind) const
    {
        const static MCFixupKindInfo Infos[SVM::NumTargetFixupKinds] = {
            DEFINE_FIXUP_KIND_INFO
        };

        if (Kind < FirstTargetFixupKind)
            return MCAsmBackend::getFixupKindInfo(Kind);
        assert(unsigned(Kind - FirstTargetFixupKind) < getNumFixupKinds() &&
               "Invalid kind!");
        return Infos[Kind - FirstTargetFixupKind];
    }

    int64_t adjustFixup(const MCFixup &Fixup, int64_t Value) const
    {
        const MCFixupKindInfo &KI = getFixupKindInfo(Fixup.getKind());

        if (KI.Flags & MCFixupKindInfo::FKF_IsPCRel) {
            // Offset due to ARM pipelining
            Value -= 4;
        }

        switch (Fixup.getKind()) {

        case SVM::fixup_bcc:
        case SVM::fixup_b:
            // Halfword count
            Value /= 2;
            break;
            
        case SVM::fixup_cpi:
            // Word count
            Value /= 4;
            break;
            
        default:
            break;
        }

        return Value;
    }

    void ApplyFixup(const MCFixup &Fixup, char *Data, unsigned DataSize,
                    uint64_t Value) const
    {
        const MCFixupKindInfo &KI = getFixupKindInfo(Fixup.getKind());

        Value = adjustFixup(Fixup, Value);

        int bits = KI.TargetSize;
        uint64_t bitMask = (1 << bits) - 1;
        assert((Value & ~bitMask) == 0 || (Value | bitMask) == (uint64_t)-1);
        Value &= bitMask;

        unsigned offset = Fixup.getOffset();
        while (bits > 0) {
            Data[offset] |= Value;
            bits -= 8;
            offset++;
            Value >>= 8;
        }
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
        
        // Our ISA only has one kind of nop currently
        for (unsigned i = 0, c = Count/2; i < c; i++)
            OW->Write16(0xbf00);

        return true;
    }
};

}  // end namespace

MCAsmBackend *llvm::createSVMAsmBackend(const Target &T, StringRef TT)
{
    Triple TheTriple(TT);
    return new SVMAsmBackend(T, Triple(TT).getOS());
}
