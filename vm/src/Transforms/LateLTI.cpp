/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "LogTransform.h"
#include "ErrorReporter.h"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Constants.h"
#include "llvm/Type.h"
#include "llvm/LLVMContext.h"
#include "llvm/Instructions.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Analysis/ValueTracking.h"
using namespace llvm;

namespace llvm {
    BasicBlockPass *createLateLTIPass();
}

namespace {
    class LateLTIPass : public BasicBlockPass {
    public:
        static char ID;
        LateLTIPass()
            : BasicBlockPass(ID) {}

        virtual bool runOnBasicBlock (BasicBlock &BB);

        virtual const char *getPassName() const {
            return "Late link-time intrinsics";
        }
        
    private:
        bool runOnCall(CallSite &CS, StringRef Name);
        void metadataString(CallSite &CS);
    };
}

char LateLTIPass::ID = 0;

BasicBlockPass *llvm::createLateLTIPass()
{
    return new LateLTIPass();
}

bool LateLTIPass::runOnBasicBlock (BasicBlock &BB)
{
    bool Changed = false;

    for (BasicBlock::iterator I = BB.begin(), E = BB.end(); I != E;) {
        CallSite CS(cast<Value>(I));
        ++I;

        if (CS) {
            Function *F = CS.getCalledFunction();
            if (F && runOnCall(CS, F->getName()))
                Changed = true;
        }
    }

    return Changed;
}

bool LateLTIPass::runOnCall(CallSite &CS, StringRef Name)
{
    if (Name == "_SYS_lti_log") {
        LogTransform(CS);
        return true;
    }
    
    if (Name == "_SYS_lti_metadata_str") {
        metadataString(CS);
    }

    return false;
}

void LateLTIPass::metadataString(CallSite &CS)
{
    Instruction *I = CS.getInstruction();
    
    if (CS.arg_size() != 2)
        report_fatal_error(I, "Wrong number of arguments for _SYS_lti_metadata_str");

    ConstantInt *CIType = dyn_cast<ConstantInt>(CS.getArgument(0));
    if (!CIType)
        report_fatal_error(I, "Metadata type must be a constant integer.");
    uint16_t type = CIType->getZExtValue();
    if (type != CIType->getZExtValue())
        report_fatal_error(I, "Metadata type argument is too large.");

    std::string str;
    if (!GetConstantStringInfo(CS.getArgument(1), str))
        report_fatal_error(I, "String for _SYS_lti_metadata_str() is not verifiably constant.");

    printf("Metadata: %d '%s'\n", type, str.c_str());

    CS.getInstruction()->eraseFromParent();
}
