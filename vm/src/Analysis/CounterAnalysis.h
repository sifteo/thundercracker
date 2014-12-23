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
