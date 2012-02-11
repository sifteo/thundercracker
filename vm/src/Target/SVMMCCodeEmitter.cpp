/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMMCTargetDesc.h"
#include "SVMFixupKinds.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCSymbol.h"
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
    MCContext &Ctx;

public:
    SVMMCCodeEmitter(const MCInstrInfo &mcii, const MCSubtargetInfo &sti, MCContext &ctx)
        : MCII(mcii), STI(sti), Ctx(ctx) {}

    ~SVMMCCodeEmitter() {}

    // Generated code
    unsigned getBinaryCodeForInstr(const MCInst &MI, SmallVectorImpl<MCFixup> &Fixups) const;

    unsigned getMachineOpValue(const MCInst &MI,const MCOperand &MO,
        SmallVectorImpl<MCFixup> &Fixups) const
    {
        if (MO.isReg()) {
            unsigned N = MO.getReg();
            if (N >= SVM::R0 && N <= SVM::R7)
                return N - SVM::R0;
        }

        if (MO.isImm())
            return static_cast<unsigned>(MO.getImm());

        llvm_unreachable("Unable to encode MCOperand");
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
        unsigned Op = MI.getOpcode();
        
        switch (Op) {
        
        case SVM::ADJCALLSTACKDOWN:
        case SVM::ADJCALLSTACKUP:
            /*
             * Some pseudoinstructions are illegal if they make it this far!
             */
            llvm_unreachable("Illegal pseudo-instruction remaining! Check"
                " the -asm outputs for opcodes beginning with '!'.");
            break;
        
        case SVM::FNSTACK_R: 
        case SVM::FNSTACK: {
            /*
             * The FNSTACK pseudo-op is used, at the top of each function,
             * to annotate the amount of stack adjustment required on entry.
             *
             * FNSTACK ops are added during frame lowering. Since we need to
             * pass them on to the linker, so that it can assemble the proper
             * function addresses, we create a target-specific fixup here.
             *
             * XXX: We use an expression containing a dummy symbol here,
             *      to ensure the fixup isn't relaxed by MCAssembler before
             *      our linker has a chance to see it. Bleh. This will convert
             *      to an MCValue with our Adj available via getConstant()
             */

            unsigned Adj = MI.getOperand(0).getImm();
            const MCExpr *AdjExpr = MCConstantExpr::Create(Adj, Ctx);
            const MCExpr *SymExpr = MCSymbolRefExpr::Create(Ctx.CreateTempSymbol(), Ctx);
            const MCExpr *E = MCBinaryExpr::CreateAdd(AdjExpr, SymExpr, Ctx);
            Fixups.push_back(MCFixup::Create(0, E, MCFixupKind(SVM::fixup_fnstack)));
            break;
        }
            
        default: {
            // Encode a normal instruction (16 or 32 bits)
            const MCInstrDesc &Desc = MCII.get(MI.getOpcode());
            int Size = Desc.getSize();
            if (Size > 0) {
                uint32_t Binary = getBinaryCodeForInstr(MI, Fixups);
                EmitConstant(Binary, Size, OS);
            }
            break;
        }
        }
    }
    
    uint32_t simpleFixup(const MCInst &MI, unsigned OpIdx,
        SmallVectorImpl<MCFixup> &Fixups, SVM::Fixups kind) const
    {
        const MCOperand MO = MI.getOperand(OpIdx);

        if (MO.isExpr()) {
            Fixups.push_back(MCFixup::Create(0, MO.getExpr(),
                MCFixupKind(kind)));
            return 0;
        }

        return MO.getImm() >> 1;
    }
    
    uint32_t getBCCTargetOpValue(const MCInst &MI, unsigned OpIdx,
        SmallVectorImpl<MCFixup> &Fixups) const
    {
        return simpleFixup(MI, OpIdx, Fixups, SVM::fixup_bcc);
    }

    uint32_t getBTargetOpValue(const MCInst &MI, unsigned OpIdx,
        SmallVectorImpl<MCFixup> &Fixups) const
    {
        return simpleFixup(MI, OpIdx, Fixups, SVM::fixup_b);
    }

    uint32_t getRelCPIOpValue(const MCInst &MI, unsigned OpIdx,
        SmallVectorImpl<MCFixup> &Fixups) const
    {
        return simpleFixup(MI, OpIdx, Fixups, SVM::fixup_relcpi);
    }

    uint32_t getAbsCPIOpValue(const MCInst &MI, unsigned OpIdx,
        SmallVectorImpl<MCFixup> &Fixups) const
    {
        return simpleFixup(MI, OpIdx, Fixups, SVM::fixup_abscpi);
    }
    
    uint32_t getAddrSPValue(const MCInst &MI, unsigned OpIdx,
        SmallVectorImpl<MCFixup> &Fixups) const
    {
        assert(MI.getNumOperands() > OpIdx + 1);
        const MCOperand &baseFI = MI.getOperand(OpIdx);
        const MCOperand &offset = MI.getOperand(OpIdx + 1);
        unsigned value = baseFI.getImm() + offset.getImm();

        assert((value & 3) == 0 &&
            "LDRsp, STRsp, and ADDsp require 4-byte-aligned values");

        return value;
    }
};

}  // end namespace

MCCodeEmitter *llvm::createSVMMCCodeEmitter(const MCInstrInfo &MCII,
    const MCSubtargetInfo &STI, MCContext &Ctx)
{
    return new SVMMCCodeEmitter(MCII, STI, Ctx);
}

#include "SVMGenMCCodeEmitter.inc"
