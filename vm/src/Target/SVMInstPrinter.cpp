/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
    OS << '%' << LowercaseString(getRegisterName(RegNo));
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
