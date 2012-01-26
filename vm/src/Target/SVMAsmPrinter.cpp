/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#define DEBUG_TYPE "asm-printer"
#include "SVM.h"
#include "SVMInstrInfo.h"
#include "SVMTargetMachine.h"
#include "SVMMCTargetDesc.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Target/Mangler.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

namespace {
    class SVMAsmPrinter : public AsmPrinter {
    public:
        explicit SVMAsmPrinter(TargetMachine &TM, MCStreamer &Streamer)
            : AsmPrinter(TM, Streamer) {}

        virtual const char *getPassName() const {
            return "SVM Assembly Printer";
        }

        virtual void EmitInstruction(const MachineInstr *MI) {
            SmallString<128> Str;
            raw_svector_ostream OS(Str);
            printInstruction(MI, OS);
            OutStreamer.EmitRawText(OS.str());
        }

        void printOperand(const MachineInstr *MI, int opNum, raw_ostream &OS);
        void printCCOperand(const MachineInstr *MI, int opNum, raw_ostream &OS);

        // Generated code
        void printInstruction(const MachineInstr *MI, raw_ostream &OS);
        static const char *getRegisterName(unsigned RegNo);
        static const char *getInstructionName(unsigned Opcode);
    };
}

#include "SVMGenAsmWriter.inc"

extern "C" void LLVMInitializeSVMAsmPrinter() { 
    RegisterAsmPrinter<SVMAsmPrinter> X(TheSVMTarget);
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


