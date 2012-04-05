/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Since it's fairly expensive to modify SP on our system, it's better to just
 * convert these dynamic stack allocations into static when possible.
 *
 * We do this by moving all alloca instructions to the first basic block
 * of the function.
 */

#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Instructions.h"
using namespace llvm;

namespace llvm {
    FunctionPass *createStaticAllocaPass();
}

namespace {
    class StaticAllocaPass : public FunctionPass {
    public:
        static char ID;
        StaticAllocaPass()
            : FunctionPass(ID) {}

        virtual bool runOnFunction (Function &F);

        virtual const char *getPassName() const {
            return "Static stack allocation pass";
        }
    };
}

char StaticAllocaPass::ID = 0;

FunctionPass *llvm::createStaticAllocaPass()
{
    return new StaticAllocaPass();
}

bool StaticAllocaPass::runOnFunction (Function &F)
{
    Function::iterator BB = F.begin(), EB = F.end();
    if (BB == EB)
        return false;

    bool Changed = false;
    BasicBlock *FirstBB = BB;
    ++BB;

    // Search all non-first BBs for alloca instructions
    for (;BB != EB; ++BB)
        for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E;) {
            AllocaInst *AI = dyn_cast<AllocaInst>(I);
            ++I;
    
            if (AI) {
                AI->moveBefore(FirstBB->begin());
                Changed = true;
            }
        }

    return Changed;
}
