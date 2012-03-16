/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
using namespace llvm;

namespace llvm {

class CallSite;

void initializeCounterAnalysisPass(PassRegistry&);

class CounterAnalysis : public BasicBlockPass {
public:
    static char ID;
    CounterAnalysis() : BasicBlockPass(ID) {
        initializeCounterAnalysisPass(*PassRegistry::getPassRegistry());
    }

    virtual bool runOnBasicBlock (BasicBlock &BB);

    virtual const char *getPassName() const {
        return "Counter analysis pass";
    }

    unsigned getValueFor(CallSite &CS) const;
    
private:

};

}  // end namespace llvm
