/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo VM (SVM) Target for LLVM
 *
 * Micah Elizabeth Scott <micah@misc.name>
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
        MCOperand LowerSymbolOperand(const MachineOperand &MO,
                                     MachineOperandType MOTy,
                                     unsigned Offset) const;
    };
}

#endif
