/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "Support/ErrorReporter.h"
#include "Analysis/CounterAnalysis.h"
#include "Analysis/UUIDGenerator.h"
#include "Target/SVMSymbolDecoration.h"
#include "llvm/ADT/SmallVector.h"
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
            AU.addRequired<UUIDGenerator>();
        }

        virtual const char *getPassName() const {
            return "Early link-time intrinsics";
        }
        
    private:
        bool runOnCall(CallSite &CS, StringRef Name);
        void replaceWithConstant(CallSite &CS, Constant *value);
        void replaceWithConstant(CallSite &CS, uint32_t value);
        void handleGetInitializer(CallSite &CS);
        void handleIsConstant(CallSite &CS);
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

void EarlyLTIPass::replaceWithConstant(CallSite &CS, Constant *value)
{
    Instruction *I = CS.getInstruction();
    I->replaceAllUsesWith(value);
    I->eraseFromParent();
}

void EarlyLTIPass::replaceWithConstant(CallSite &CS, uint32_t value)
{
    Instruction *I = CS.getInstruction();
    LLVMContext &Ctx = I->getContext();
    Constant *C = ConstantInt::get(Type::getInt32Ty(Ctx), value);
    replaceWithConstant(CS, ConstantExpr::getIntegerCast(C, I->getType(), false));
}

void EarlyLTIPass::handleGetInitializer(CallSite &CS)
{
    Instruction *I = CS.getInstruction();
    Module *M = I->getParent()->getParent()->getParent();

    if (CS.arg_size() != 1)
        report_fatal_error(I, "_SYS_lti_initializer requires exactly one arg");

    // The argument likely has casts on it, to take it to void*. Remove those.
    Value *Arg = CS.getArgument(0);
    Arg = Arg->stripPointerCasts();

    // See if we can actually look up the GV initializer
    GlobalVariable *GV = dyn_cast<GlobalVariable>(Arg);
    if (!GV)
        report_fatal_error(I, "Argument to _SYS_lti_initializer must be a GlobalVariable");
    if (!GV->hasInitializer())
        report_fatal_error(I, "Argument to _SYS_lti_initializer has no initializer");
    Constant *Init = GV->getInitializer();

    // The initializer is a raw Constant, and its address can't be taken.
    // We need to create a new GlobalVariable to hold the initializer itself,
    // so that we can return it as a pointer.
    
    GlobalVariable *InitGV = new GlobalVariable(*M, Init->getType(),
        true, GlobalValue::PrivateLinkage, Init,
        Twine(SVMDecorations::INIT) + GV->getName(),
        0, false);

    // Bitcast the result back to the expected type (usually void*)
    replaceWithConstant(CS, ConstantExpr::getPointerCast(InitGV, I->getType()));
}

void EarlyLTIPass::handleIsConstant(CallSite &CS)
{
    if (CS.arg_size() != 1)
        report_fatal_error(CS.getInstruction(), "_SYS_lti_isConstant requires exactly one arg");

    Constant *C = dyn_cast<Constant>(CS.getArgument(0));
    replaceWithConstant(CS, uint32_t(C != 0));
}

bool EarlyLTIPass::runOnCall(CallSite &CS, StringRef Name)
{
    if (Name == "_SYS_lti_isDebug") {
        replaceWithConstant(CS, ELFDebug);
        return true;
    }

    if (Name == "_SYS_lti_counter") {
        replaceWithConstant(CS, getAnalysis<CounterAnalysis>().getValueFor(CS));
        return true;
    }

    if (Name == "_SYS_lti_uuid") {
        replaceWithConstant(CS, getAnalysis<UUIDGenerator>().getValueFor(CS));
        return true;
    }

    if (Name == "_SYS_lti_initializer") {
        handleGetInitializer(CS);
        return true;
    }

    if (Name == "_SYS_lti_isConstant") {
        handleIsConstant(CS);
        return true;
    }

    return false;
}
