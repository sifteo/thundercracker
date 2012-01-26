/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMMCInstLower.h"
#include "SVMAsmPrinter.h"
#include "SVMInstrInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Target/Mangler.h"
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
    return MCOperand();
}

