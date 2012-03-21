/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_ASMPRINTER_H
#define SVM_ASMPRINTER_H
#include "llvm/MC/MCInstPrinter.h"

namespace llvm {
    
    class SVMInstPrinter : public MCInstPrinter {
    public:
        SVMInstPrinter(const MCAsmInfo &MAI) : MCInstPrinter(MAI) {}

        // Generated code
        void printInstruction(const MCInst *MI, raw_ostream &O);
        static const char *getInstructionName(unsigned Opcode);
        static const char *getRegisterName(unsigned RegNo);

        virtual StringRef getOpcodeName(unsigned Opcode) const;
        virtual void printRegName(raw_ostream &OS, unsigned RegNo) const;
        virtual void printInst(const MCInst *MI, raw_ostream &O, StringRef Annot);

        // Custom print functions
        void printAddrSPValue(const MCInst *MI, unsigned OpNum, raw_ostream &O);

    private:
        static const char *getCCName(int cc);
        void printOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O);
        void printCCOperand(const MCInst *MI, int opNum, raw_ostream &O);
    };
    
} // end namespace

#endif
