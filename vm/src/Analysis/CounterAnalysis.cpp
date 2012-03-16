/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "CounterAnalysis.h"
#include "llvm/Pass.h"
using namespace llvm;

char CounterAnalysis::ID = 0;
INITIALIZE_PASS(CounterAnalysis, "counter-analysis",
                "Counter analysis pass", false, true)


bool CounterAnalysis::runOnBasicBlock (BasicBlock &BB)
{
    return false;
}

unsigned CounterAnalysis::getValueFor(CallSite &CS) const
{
    return 5;
}
