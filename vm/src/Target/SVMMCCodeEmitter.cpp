/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMMCTargetDesc.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

namespace {

class SVMMCCodeEmitter : public MCCodeEmitter {
    SVMMCCodeEmitter(const SVMMCCodeEmitter &);     // Leave unimplemented!
    void operator=(const SVMMCCodeEmitter &);       // Leave unimplemented!
    const MCInstrInfo &MCII;
    const MCSubtargetInfo &STI;

public:
    SVMMCCodeEmitter(const MCInstrInfo &mcii, const MCSubtargetInfo &sti, MCContext &ctx)
        : MCII(mcii), STI(sti) {}

    ~SVMMCCodeEmitter() {}

    // Generated code
    unsigned getBinaryCodeForInstr(const MCInst &MI, SmallVectorImpl<MCFixup> &Fixups) const;

    unsigned getMachineOpValue(const MCInst &MI,const MCOperand &MO,
        SmallVectorImpl<MCFixup> &Fixups) const
    {
        if (MO.isReg())
            return MO.getReg() - SVM::R0;

        if (MO.isImm())
            return static_cast<unsigned>(MO.getImm());

        return 0;
    }

    void EmitByte(unsigned char C, raw_ostream &OS) const
    {
        OS << (char)C;
    }

    void EmitConstant(uint64_t Val, unsigned Size, raw_ostream &OS) const
    {
        for (unsigned i = 0; i != Size; ++i) {
            // Little endian
            EmitByte(Val & 255, OS);
            Val >>= 8;
        }
    }

    void EncodeInstruction(const MCInst &MI, raw_ostream &OS,
        SmallVectorImpl<MCFixup> &Fixups) const
    {
        const MCInstrDesc &Desc = MCII.get(MI.getOpcode());
        int Size = Desc.getSize();
        if (Size > 0) {
            uint32_t Binary = getBinaryCodeForInstr(MI, Fixups);
            EmitConstant(Binary, Size, OS);
        }
    }
};

}  // end namespace

MCCodeEmitter *llvm::createSVMMCCodeEmitter(const MCInstrInfo &MCII,
    const MCSubtargetInfo &STI, MCContext &Ctx)
{
    return new SVMMCCodeEmitter(MCII, STI, Ctx);
}

#include "SVMGenMCCodeEmitter.inc"
