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

        void printOperand(const MachineInstr *MI, int opNum, raw_ostream &OS);

        virtual void EmitInstruction(const MachineInstr *MI) {
            SmallString<128> Str;
            raw_svector_ostream OS(Str);
            printInstruction(MI, OS);
            OutStreamer.EmitRawText(OS.str());
        }

        void printInstruction(const MachineInstr *MI, raw_ostream &OS);
        static const char *getRegisterName(unsigned RegNo);
    };
}

#include "SVMGenAsmWriter.inc"

void SVMAsmPrinter::printOperand(const MachineInstr *MI, int opNum, raw_ostream &O)
{
}

extern "C" void LLVMInitializeSVMAsmPrinter() { 
    RegisterAsmPrinter<SVMAsmPrinter> X(TheSVMTarget);
}
