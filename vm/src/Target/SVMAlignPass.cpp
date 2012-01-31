/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVM.h"
#include "SVMTargetMachine.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
using namespace llvm;

namespace {

    class SVMAlignPass : public MachineFunctionPass {
        SVMTargetMachine& TM;
        public:
    public:
        static char ID;
        explicit SVMAlignPass(SVMTargetMachine &tm)
            : MachineFunctionPass(ID), TM(tm) {}

        virtual bool runOnMachineFunction(MachineFunction &MF);

        virtual const char *getPassName() const {
            return "SVM alignment pass";
        }
    };
  
    char SVMAlignPass::ID = 0;
}

FunctionPass *llvm::createSVMAlignPass(SVMTargetMachine &TM)
{
    return new SVMAlignPass(TM);
}

bool SVMAlignPass::runOnMachineFunction(MachineFunction &MF)
{
    for (MachineFunction::iterator I = MF.begin(), E = MF.end(); I != E; ++I) {
        MachineBasicBlock &MBB = *I;

        // All basic blocks must align to a bundle boundary
        MBB.setAlignment(std::max(MBB.getAlignment(), TM.getBundleSize()));
    }

    return true;
}
