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

    void EncodeInstruction(const MCInst &MI, raw_ostream &OS,
        SmallVectorImpl<MCFixup> &Fixups) const
    {
    }
    
};

}  // end namespace

MCCodeEmitter *llvm::createSVMMCCodeEmitter(const MCInstrInfo &MCII,
    const MCSubtargetInfo &STI, MCContext &Ctx)
{
    return new SVMMCCodeEmitter(MCII, STI, Ctx);
}
