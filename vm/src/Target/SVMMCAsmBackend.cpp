/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMMCAsmBackend.h"
using namespace llvm;


MCObjectWriter *SVMAsmBackend::createObjectWriter(raw_ostream &OS) const
{
    return createSVMELFProgramWriter(OS);
}

unsigned SVMAsmBackend::getNumFixupKinds() const
{
    return SVM::NumTargetFixupKinds;
}

const MCFixupKindInfo &SVMAsmBackend::getStaticFixupKindInfo(MCFixupKind Kind)
{
    /*
     * XXX: This duplicates functionality from MCAsmBackend::getFixupKindInfo
     *      in order to provide a static function that can be used in our
     *      SVMELFProgramWriter.
     */
    
    static const MCFixupKindInfo Builtins[] = {
      { "FK_Data_1", 0, 8, 0 },
      { "FK_Data_2", 0, 16, 0 },
      { "FK_Data_4", 0, 32, 0 },
      { "FK_Data_8", 0, 64, 0 },
      { "FK_PCRel_1", 0, 8, MCFixupKindInfo::FKF_IsPCRel },
      { "FK_PCRel_2", 0, 16, MCFixupKindInfo::FKF_IsPCRel },
      { "FK_PCRel_4", 0, 32, MCFixupKindInfo::FKF_IsPCRel },
      { "FK_PCRel_8", 0, 64, MCFixupKindInfo::FKF_IsPCRel }
    };

    const static MCFixupKindInfo Locals[SVM::NumTargetFixupKinds] = {
        DEFINE_FIXUP_KIND_INFO
    };

    size_t Index = (size_t)Kind;

    if (Kind < FirstTargetFixupKind) {
        assert(Index < sizeof Builtins / sizeof Builtins[0] &&
            "Unknown fixup kind");
        return Builtins[Index];
    }
    
    Index -= FirstTargetFixupKind;
    assert(Index < sizeof Locals / sizeof Locals[0] &&
        "Unknown fixup kind");
    return Locals[Index];
}

const MCFixupKindInfo &SVMAsmBackend::getFixupKindInfo(MCFixupKind Kind) const
{
    return getStaticFixupKindInfo(Kind);
}

void SVMAsmBackend::ApplyStaticFixup(MCFixupKind Kind, char *Data, int32_t Value)
{
    const MCFixupKindInfo &KI = getStaticFixupKindInfo(Kind);
    int Bits = KI.TargetSize;
    if (!Bits)
        return;

    switch (Kind) {

    case SVM::fixup_bcc:
    case SVM::fixup_b:
        // PC-relative halfword count
        Value = (Value - 4) / 2;
        break;

    case SVM::fixup_relcpi:
        // Aligned-PC-relative word count
        Value = (Value - 2) / 4;
        break;

    case SVM::fixup_abscpi:
        // Word count from beginning of block
        Value = (Value / 4) & 0x7F;
        break;

    default:
        break;
    }

    uint32_t BitMask = ((uint64_t)1 << Bits) - 1;
    assert((Value & ~BitMask) == 0 || (Value | BitMask) == 0xFFFFFFFF);
    Value &= BitMask;

    do {
        *Data |= Value;
        Bits -= 8;
        Data++;
        Value >>= 8;
    } while (Bits > 0);
}

void SVMAsmBackend::ApplyFixup(const MCFixup &Fixup, char *Data,
    unsigned DataSize, uint64_t Value) const
{
    ApplyStaticFixup(Fixup.getKind(), Data + Fixup.getOffset(), Value);
}

bool SVMAsmBackend::MayNeedRelaxation(const MCInst &Inst) const
{
    switch (Inst.getOpcode()) {

    /*
     * XXX: Put FNSTACK in its own MCFragment, since LLVM doesn't handle
     *      overlapping fixups within the same fragment, and it's quite
     *      possible for our special zero-size FNSTACK fixup to coincide
     *      with a real fixup. We do this by running FNSTACK through
     *      RelaxInstruction once.
     */
    case SVM::FNSTACK:
        return true;

    default:
        return false;
    }
}

void SVMAsmBackend::RelaxInstruction(const MCInst &Inst, MCInst &Res) const
{
    Res = Inst;
    switch (Inst.getOpcode()) {
        
    case SVM::FNSTACK:
        Res.setOpcode(SVM::FNSTACK_R);
        break;
    
    default:
        assert(0 && "Not implemented");
    }
}

bool SVMAsmBackend::WriteNopData(uint64_t Count, MCObjectWriter *OW) const
{
    assert((Count % 2) == 0);
    
    // Our ISA only has one kind of nop currently
    for (unsigned i = 0, c = Count/2; i < c; i++)
        OW->Write16(0xbf00);

    return true;
}

MCAsmBackend *llvm::createSVMAsmBackend(const Target &T, StringRef TT)
{
    Triple TheTriple(TT);
    return new SVMAsmBackend(T, Triple(TT).getOS());
}
