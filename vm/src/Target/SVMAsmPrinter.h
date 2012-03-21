/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_ASMPRINTER_H
#define SVM_ASMPRINTER_H

#include "SVMMCInstlower.h"
#include "SVMBlockSizeAccumulator.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"

namespace llvm {

    class SVMAsmPrinter : public AsmPrinter {
    public:
        explicit SVMAsmPrinter(TargetMachine &TM, MCStreamer &Streamer)
            : AsmPrinter(TM, Streamer), CurrentMBB(0) {}

        const char *getPassName() const {
            return "SVM Assembly Printer";
        }

        void EmitInstruction(const MachineInstr *MI);
        void EmitFunctionEntryLabel();
        void EmitConstantPool();
        void EmitFunctionBodyEnd();
        void EmitMachineConstantPoolValue(MachineConstantPoolValue *MCPV);

    private:
        struct CPEInfo {
            CPEInfo(MCSymbol *Symbol, const MachineConstantPoolEntry *MCPE)
                : Symbol(Symbol), MCPE(MCPE) {}

            MCSymbol *Symbol;
            const MachineConstantPoolEntry *MCPE;
        };

        typedef DenseMap<const MCSymbol*, CPEInfo> BlockConstPoolTy;
        BlockConstPoolTy BlockConstPool;

        unsigned CurrentFnSplitOrdinal;
        SVMBlockSizeAccumulator BSA;
        const MachineBasicBlock *CurrentMBB;

        void emitBlockBegin();
        void emitBlockEnd();
        void emitBlockSplit(const MachineInstr *MI);
        void emitFunctionLabelImpl(MCSymbol *Sym);
        void emitBlockOffsetComment();

        void emitBlockConstPool();
        void emitConstRefComment(const MachineOperand &MO);
        void rewriteConstForCurrentBlock(const MachineOperand &MO, MCOperand &MCO);
    };

} // end namespace

#endif
