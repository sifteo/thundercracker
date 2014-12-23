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

#include "SVMInstPrinter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/StringExtras.h"
using namespace llvm;

#define GET_INSTRUCTION_NAME
#include "SVMGenAsmWriter.inc"


const char *SVMInstPrinter::getCCName(int cc)
{
    static const char *ccNames[] = {
        "eq", "ne", "hs", "lo", "mi", "pl", "vs", "vc",
        "hi", "ls", "ge", "lt", "gt", "le", "al"
    };  
    unsigned uCC = (unsigned)cc;
    assert(uCC < sizeof ccNames / sizeof ccNames[0]);
    return ccNames[uCC];
}

StringRef SVMInstPrinter::getOpcodeName(unsigned Opcode) const
{
    return getInstructionName(Opcode);
}

void SVMInstPrinter::printRegName(raw_ostream &OS, unsigned RegNo) const
{
    OS << LowercaseString(getRegisterName(RegNo));
}

void SVMInstPrinter::printInst(const MCInst *MI, raw_ostream &O, StringRef Annot)
{
    printInstruction(MI, O);
    printAnnotation(O, Annot);
}

void SVMInstPrinter::printOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O)
{
    const MCOperand &Op = MI->getOperand(OpNo);

    if (Op.isReg()) {
        printRegName(O, Op.getReg());
        return;
    }

    if (Op.isImm()) {
        O << Op.getImm();
        return;
    }

    assert(Op.isExpr() && "Unknown operand type in printOperand");
    O << *Op.getExpr();
}

void SVMInstPrinter::printCCOperand(const MCInst *MI, int opNum, raw_ostream &O)
{
    const MCOperand& MO = MI->getOperand(opNum);
    O << getCCName((int)MO.getImm());
}

void SVMInstPrinter::printAddrSPValue(const MCInst *MI, unsigned OpNum, raw_ostream &O)
{
    const MCOperand &baseFI = MI->getOperand(OpNum);
    const MCOperand &offset = MI->getOperand(OpNum + 1);
    O << (baseFI.getImm() + offset.getImm());
}
