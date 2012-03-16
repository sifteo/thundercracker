/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "Analysis/CounterAnalysis.h"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Constants.h"
#include "llvm/Type.h"
#include "llvm/LLVMContext.h"
#include "llvm/Instructions.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/CommandLine.h"
using namespace llvm;

namespace llvm {
    BasicBlockPass *createEarlyLTIPass();
}

extern cl::opt<bool> ELFDebug;

namespace {
    class EarlyLTIPass : public BasicBlockPass {
    public:
        static char ID;
        EarlyLTIPass()
            : BasicBlockPass(ID) {}

        virtual bool runOnBasicBlock (BasicBlock &BB);

        void getAnalysisUsage(AnalysisUsage &AU) const {
            AU.addRequired<CounterAnalysis>();
        }

        virtual const char *getPassName() const {
            return "Early link-time intrinsics";
        }
        
    private:
        bool runOnCall(CallSite &CS, StringRef Name);
        void replaceWithConstant(CallSite &CS, uint32_t value);
    };
}

char EarlyLTIPass::ID = 0;

BasicBlockPass *llvm::createEarlyLTIPass()
{
    return new EarlyLTIPass();
}

bool EarlyLTIPass::runOnBasicBlock (BasicBlock &BB)
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

void EarlyLTIPass::replaceWithConstant(CallSite &CS, uint32_t value)
{
    Instruction *I = CS.getInstruction();
    LLVMContext &Ctx = I->getContext();
    Value *C = ConstantInt::get(Type::getInt32Ty(Ctx), value);
    I->replaceAllUsesWith(C);
    I->eraseFromParent();
}

bool EarlyLTIPass::runOnCall(CallSite &CS, StringRef Name)
{
    if (Name == "_SYS_lti_isDebug") {
        replaceWithConstant(CS, ELFDebug);
        return true;
    }

    if (Name == "_SYS_lti_counter") {
        const CounterAnalysis &CA = getAnalysis<CounterAnalysis>();
        replaceWithConstant(CS, CA.getValueFor(CS));
        return true;
    }

    return false;
}
