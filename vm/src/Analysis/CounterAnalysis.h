/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include "llvm/ADT/SmallVector.h"
#include <utility>
#include <map>
using namespace llvm;

namespace llvm {

class CallSite;
class Instruction;

void initializeCounterAnalysisPass(PassRegistry&);

class CounterAnalysis : public ModulePass {
public:
    static char ID;
    CounterAnalysis() : ModulePass(ID) {
        initializeCounterAnalysisPass(*PassRegistry::getPassRegistry());
    }

    virtual bool runOnModule(Module &M);

    virtual const char *getPassName() const {
        return "Counter analysis pass";
    }

    void getAnalysisUsage(AnalysisUsage &AU) const {
        AU.setPreservesAll();
    }

    unsigned getValueFor(CallSite &CS) const;

private:
    typedef std::pair<std::string, int64_t> Key_t;
    typedef std::multimap<Key_t, const Instruction*> KeyMap_t;
    typedef std::map<const Instruction*, unsigned> ValueMap_t;

    KeyMap_t KeyMap;
    ValueMap_t ValueMap;

    void keyUnpack(CallSite &CS, Key_t &k);

    void runOnFunction(Function &F);
    void runOnBasicBlock(BasicBlock &BB);
    void runOnCall(CallSite &CS);
    
    void assignValues();
};

}  // end namespace llvm
