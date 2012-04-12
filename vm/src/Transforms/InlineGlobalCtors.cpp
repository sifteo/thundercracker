/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Constants.h"
#include "llvm/Type.h"
#include "llvm/LLVMContext.h"
#include "llvm/Instructions.h"
#include "llvm/Attributes.h"
using namespace llvm;

namespace llvm {
    ModulePass *createInlineGlobalCtorsPass();
}

namespace {
    class InlineGlobalCtorsPass : public ModulePass {
    public:
        static char ID;
        InlineGlobalCtorsPass()
            : ModulePass(ID) {}
        
        virtual bool runOnModule(Module &M);

        virtual const char *getPassName() const {
            return "Inlining global constructors";
        }
        
    private:
        void createStubAtExit(Module &M);
        void removeCtorDtorLists(Module &M);
        void inlineCtorList(Module &M);
    };
}

char InlineGlobalCtorsPass::ID = 0;


bool InlineGlobalCtorsPass::runOnModule(Module &M)
{
    createStubAtExit(M);
    inlineCtorList(M);
    removeCtorDtorLists(M);

    return true;
}

void InlineGlobalCtorsPass::inlineCtorList(Module &M)
{
    // All constructor pointers are in a global constant array
    GlobalVariable *GV = M.getGlobalVariable("llvm.global_ctors");
    if (!GV)
        return;
    assert(GV->hasUniqueInitializer());
    if (isa<ConstantAggregateZero>(GV->getInitializer()))
        return;

    // Operate on a new basic block, prepended to main()
    Function *Main = M.getFunction("main");
    if (!Main)
        return;
    LLVMContext &Ctx = M.getContext();
    BasicBlock *EntryBB = &Main->getEntryBlock();
    BasicBlock *CtorBB = BasicBlock::Create(Ctx, "CtorBlock", Main, EntryBB);

    // Add each constructor, in order.
    ConstantArray *CA = cast<ConstantArray>(GV->getInitializer());
    for (User::op_iterator i = CA->op_begin(), e = CA->op_end(); i != e; ++i) {
        ConstantStruct *CS = cast<ConstantStruct>(*i);
        Function *F = dyn_cast<Function>(CS->getOperand(1));
        if (F)
            CallInst::Create(F, "", CtorBB);
    }

    // Branch to the real main()
    BranchInst::Create(EntryBB, CtorBB);
}

void InlineGlobalCtorsPass::createStubAtExit(Module &M)
{
    /*
     * If the module has declared a _cxa_atexit function, give it a stub
     * body. We have no need to run static destructors, so stubbing out
     * this function will effectively remove such destructors from the
     * program after another round of inlining.
     */
    
    if (Function *F = M.getFunction("__cxa_atexit")) {
        LLVMContext &Ctx = M.getContext();

        // return 0;
        BasicBlock *BB = BasicBlock::Create(Ctx, "EntryBlock", F);
        Value *Zero = ConstantInt::get(Type::getInt32Ty(Ctx), 0);
        ReturnInst::Create(Ctx, Zero, BB);

        // Always expand inline, and don't include this function in the image
        F->addFnAttr(Attribute::AlwaysInline);
        F->setLinkage(GlobalValue::InternalLinkage);
    }
}

void InlineGlobalCtorsPass::removeCtorDtorLists(Module &M)
{
    /*
     * After we've finished with the global constructor/destructor lists,
     * remove them from the module. They are now redundant with the inlined
     * calls.
     */

     if (GlobalVariable *GV = M.getGlobalVariable("llvm.global_ctors"))
         GV->eraseFromParent();
     if (GlobalVariable *GV = M.getGlobalVariable("llvm.global_dtors"))
         GV->eraseFromParent();
}

ModulePass *llvm::createInlineGlobalCtorsPass()
{
    return new InlineGlobalCtorsPass();
}
