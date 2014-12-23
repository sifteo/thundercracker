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

#include "Transforms/LogTransform.h"
#include "Transforms/MetadataTransform.h"
#include "Support/ErrorReporter.h"
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
        void handleAbort(CallSite &CS);
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

void LateLTIPass::handleAbort(CallSite &CS)
{
    Instruction *I = CS.getInstruction();
    std::string msg;

    if (CS.arg_size() != 2)
        report_fatal_error(I, "_SYS_lti_abort requires exactly two args");

    // Parse the 'enable' boolean
    ConstantInt *CI = dyn_cast<ConstantInt>(CS.getArgument(0));
    if (!CI)
        report_fatal_error(I, "_SYS_lti_abort expression must be a compile-time constant");

    // Parse the 'message' parameter
    if (!GetConstantStringInfo(CS.getArgument(1), msg))
        report_fatal_error(I, "_SYS_lti_abort message must be a constant string.");

    if (CI->getZExtValue())
        report_fatal_error(I, Twine(msg));

    I->eraseFromParent();
}

bool LateLTIPass::runOnCall(CallSite &CS, StringRef Name)
{
    const TargetData *TD = getAnalysisIfAvailable<TargetData>();
    assert(TD);

    if (Name == "_SYS_lti_log") {
        LogTransform(CS);
        return true;
    }
    
    if (Name == "_SYS_lti_metadata") {
        MetadataTransform(CS, TD);
        return true;
    }
    
    if (Name == "_SYS_lti_abort") {
        handleAbort(CS);
        return true;
    }

    return false;
}
