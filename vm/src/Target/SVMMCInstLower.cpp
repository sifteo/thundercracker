/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMMCInstLower.h"
#include "SVMAsmPrinter.h"
#include "SVMInstrInfo.h"
#include "SVMMCExpr.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/Target/Mangler.h"
#include "llvm/Support/ErrorHandling.h"
using namespace llvm;

SVMMCInstLower::SVMMCInstLower(Mangler *mang, const MachineFunction &mf,
    SVMAsmPrinter &asmprinter)
    : Ctx(mf.getContext()), Mang(mang), AsmPrinter(asmprinter) {}

void SVMMCInstLower::Lower(const MachineInstr *MI, MCInst &OutMI) const
{
    OutMI.setOpcode(MI->getOpcode());
  
    for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
        const MachineOperand &MO = MI->getOperand(i);
        MCOperand MCOp = LowerOperand(MO);

        if (MCOp.isValid())
            OutMI.addOperand(MCOp);
    }
}

MCOperand SVMMCInstLower::LowerOperand(const MachineOperand& MO) const
{
    MachineOperandType MOTy = MO.getType();

    switch (MOTy) {
    default:
        assert(0 && "unknown operand type");
        break;
    case MachineOperand::MO_Register:
        if (MO.isImplicit()) break;
        return MCOperand::CreateReg(MO.getReg());
    case MachineOperand::MO_Immediate:
        return MCOperand::CreateImm(MO.getImm());
    case MachineOperand::MO_MachineBasicBlock:
    case MachineOperand::MO_GlobalAddress:
    case MachineOperand::MO_ExternalSymbol:
    case MachineOperand::MO_JumpTableIndex:
    case MachineOperand::MO_ConstantPoolIndex:
    case MachineOperand::MO_BlockAddress:
        return LowerSymbolOperand(MO, MOTy, 0);
    }

    return MCOperand();
}

MCOperand SVMMCInstLower::LowerSymbolOperand(const MachineOperand &MO,
                                             MachineOperandType MOTy,
                                             unsigned Offset) const
{
    const MCSymbol *Symbol;

    switch (MOTy) {
    default:
        llvm_unreachable("unknown operand type");
    case MachineOperand::MO_MachineBasicBlock:
        Symbol = MO.getMBB()->getSymbol();
        break;
    case MachineOperand::MO_GlobalAddress:
        Symbol = Mang->getSymbol(MO.getGlobal());
        break;
    case MachineOperand::MO_BlockAddress:
        Symbol = AsmPrinter.GetBlockAddressSymbol(MO.getBlockAddress());
        break;
    case MachineOperand::MO_ExternalSymbol:
        Symbol = AsmPrinter.GetExternalSymbolSymbol(MO.getSymbolName());
        break;
    case MachineOperand::MO_JumpTableIndex:
        Symbol = AsmPrinter.GetJTISymbol(MO.getIndex());
        break;
    case MachineOperand::MO_ConstantPoolIndex:
        Symbol = AsmPrinter.GetCPISymbol(MO.getIndex());
        if (MO.getOffset())
            Offset += MO.getOffset();  
        break;
    }

    SVMTOF::TFlags TF = (SVMTOF::TFlags) MO.getTargetFlags();
    const MCExpr *Expr =
        MCSymbolRefExpr::Create(Symbol, MCSymbolRefExpr::VK_None, Ctx);

    if (TF) {
        // Wrap in an SVMMCExpr which can hold the TargetFlags until fixup time
        Expr = SVMMCExpr::Create(TF, Expr, Ctx);
    }

    if (Offset) {
        // Add any nonzero offset
        Expr = MCBinaryExpr::CreateAdd(
            Expr, MCConstantExpr::Create(Offset, Ctx), Ctx);
    }

    return MCOperand::CreateExpr(Expr);
}
                                        