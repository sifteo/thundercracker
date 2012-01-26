/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVM.h"
#include "SVMInstrInfo.h"
#include "SVMTargetMachine.h"
#include "SVMMCTargetDesc.h"
#include "SVMMCInstLower.h"
#include "SVMAsmPrinter.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Target/Mangler.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#include "SVMGenAsmWriter.inc"

extern "C" void LLVMInitializeSVMAsmPrinter() { 
    RegisterAsmPrinter<SVMAsmPrinter> X(TheSVMTarget);
}

void SVMAsmPrinter::EmitInstruction(const MachineInstr *MI)
{
    SVMMCInstLower MCInstLowering(Mang, *MF, *this);
    MCInst low;
    MCInstLowering.Lower(MI, low);
    OutStreamer.EmitInstruction(low);
}

void SVMAsmPrinter::printOperand(const MachineInstr *MI, int opNum, raw_ostream &OS)
{
    const MachineOperand &MO = MI->getOperand(opNum);

    switch (MO.getType()) {
    case MachineOperand::MO_Register:
        OS << "%" << LowercaseString(getRegisterName(MO.getReg()));
        break;
    case MachineOperand::MO_Immediate:
        OS << (int)MO.getImm();
        break;
    case MachineOperand::MO_MachineBasicBlock:
        OS << *MO.getMBB()->getSymbol();
        return;
    case MachineOperand::MO_GlobalAddress:
        OS << *Mang->getSymbol(MO.getGlobal());
        break;
    case MachineOperand::MO_ConstantPoolIndex:
        OS << MAI->getPrivateGlobalPrefix() << "CPI"
            << getFunctionNumber() << "_" << MO.getIndex();
        break;
    default:
        llvm_unreachable("<unknown operand type>");
    }
}

void SVMAsmPrinter::printCCOperand(const MachineInstr *MI, int opNum, raw_ostream &O)
{
    int CC = (int)MI->getOperand(opNum).getImm();
    static const char *ccNames[] = {
        "eq", "ne", "hs", "lo", "mi", "pl", "vs", "vc",
        "hi", "ls", "ge", "lt", "gt", "le", "al"
    };  

    O << ccNames[CC];
}


