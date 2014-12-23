/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
