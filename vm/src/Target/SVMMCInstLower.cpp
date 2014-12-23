/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "SVMMCInstLower.h"
#include "SVMAsmPrinter.h"
#include "SVMInstrInfo.h"
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

    const MCExpr *Expr =
        MCSymbolRefExpr::Create(Symbol, MCSymbolRefExpr::VK_None, Ctx);

    if (Offset) {
        // Add any nonzero offset
        Expr = MCBinaryExpr::CreateAdd(
            Expr, MCConstantExpr::Create(Offset, Ctx), Ctx);
    }

    return MCOperand::CreateExpr(Expr);
}
                                        