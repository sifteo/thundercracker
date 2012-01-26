/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_MCINSTLOWER_H
#define SVM_MCINSTLOWER_H

#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/Support/Compiler.h"

namespace llvm {
    class MCAsmInfo;
    class MCContext;
    class MCInst;
    class MCOperand;
    class MCSymbol;
    class MachineInstr;
    class MachineFunction;
    class Mangler;
    class SVMAsmPrinter;

    class SVMMCInstLower {
    public:
        SVMMCInstLower(Mangler *mang, const MachineFunction &MF,
            SVMAsmPrinter &asmprinter);  
        void Lower(const MachineInstr *MI, MCInst &OutMI) const;

    private:
        typedef MachineOperand::MachineOperandType MachineOperandType;
        MCContext &Ctx;
        Mangler *Mang;
        SVMAsmPrinter &AsmPrinter;

        MCOperand LowerOperand(const MachineOperand& MO) const;
    };
}

#endif
