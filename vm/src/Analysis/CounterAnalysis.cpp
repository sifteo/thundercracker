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

#include "CounterAnalysis.h"
#include "Support/ErrorReporter.h"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Constants.h"
#include "llvm/Type.h"
#include "llvm/LLVMContext.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Target/TargetData.h"
using namespace llvm;

char CounterAnalysis::ID = 0;
INITIALIZE_PASS(CounterAnalysis, "counter-analysis",
                "Counter analysis pass", false, true)


bool CounterAnalysis::runOnModule(Module &M)
{
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
        runOnFunction(*I);
    assignValues();
    return false;
}

void CounterAnalysis::runOnFunction(Function &F)
{
    for (Function::iterator I = F.begin(), E = F.end(); I != E; ++I)
        runOnBasicBlock(*I);
}

void CounterAnalysis::runOnBasicBlock (BasicBlock &BB)
{
    for (BasicBlock::iterator I = BB.begin(), E = BB.end(); I != E;) {
        CallSite CS(cast<Value>(I));
        ++I;

        if (CS) {
            Function *F = CS.getCalledFunction();
            if (F && F->getName() == "_SYS_lti_counter")
                runOnCall(CS);
        }
    }
}

void CounterAnalysis::keyUnpack(CallSite &CS, Key_t &k)
{
    Instruction *I = CS.getInstruction();

    if (CS.arg_size() != 2)
        report_fatal_error(I, "_SYS_lti_counter requires two arguments");

    // Parse the 'name' parameter
    if (!GetConstantStringInfo(CS.getArgument(0), k.first))
        report_fatal_error(I, "_SYS_lti_counter name must be a constant string");
    
    // Parse the 'priority' parameter.
    ConstantInt *CI = dyn_cast<ConstantInt>(CS.getArgument(1));
    if (!CI)
        report_fatal_error(I, "_SYS_lti_counter priority must be a constant integer");

    // Inverted, so that sorting by ascending key will sort by decreasing priority.
    k.second = -CI->getSExtValue();
}

void CounterAnalysis::runOnCall(CallSite &CS)
{
    // Use KeyMap to sort all calls first by name, then by (inverse) priority.
    Key_t k;
    keyUnpack(CS, k);
    KeyMap.insert(std::make_pair(k, CS.getInstruction()));
}

void CounterAnalysis::assignValues()
{
    // Run through our sorted KeyMap, and assign counter values.

    std::string prevName;
    unsigned nextCount = 0;

    for (KeyMap_t::const_iterator I = KeyMap.begin(), E = KeyMap.end(); I != E; ++I) {
        const std::string &name = I->first.first;

        // Reset count when the name changes
        if (name != prevName)
            nextCount = 0;
        prevName = name;

        // Assign increasing numbers, in order.
        ValueMap.insert(std::make_pair(I->second, nextCount));
        nextCount++;
    }
}

unsigned CounterAnalysis::getValueFor(CallSite &CS) const
{
    ValueMap_t::const_iterator I = ValueMap.find(CS.getInstruction());
    assert(I != ValueMap.end() && "Counter call site was never seen by CounterAnalysis");
    return I->second;
}
