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

        const char *getPassName() const {
            return "SVM Assembly Printer";
        }

        void EmitInstruction(const MachineInstr *MI);
        void EmitFunctionEntryLabel();
        void EmitConstantPool();
        void EmitFunctionBodyEnd();
    };

} // end namespace

#endif
