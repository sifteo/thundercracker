/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_ASMPRINTER_H
#define SVM_ASMPRINTER_H

#include "SVMMCInstlower.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"

namespace llvm {

    class SVMAsmPrinter : public AsmPrinter {
    public:
        explicit SVMAsmPrinter(TargetMachine &TM, MCStreamer &Streamer)
            : AsmPrinter(TM, Streamer) {}

        virtual const char *getPassName() const {
            return "SVM Assembly Printer";
        }

        virtual void EmitInstruction(const MachineInstr *MI);

        void printOperand(const MachineInstr *MI, int opNum, raw_ostream &OS);
        void printCCOperand(const MachineInstr *MI, int opNum, raw_ostream &OS);

        // Generated code
        void printInstruction(const MachineInstr *MI, raw_ostream &OS);
        static const char *getRegisterName(unsigned RegNo);
        static const char *getInstructionName(unsigned Opcode);
    };

} // end namespace

#endif
