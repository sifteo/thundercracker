/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

extern "C" {
#ifdef WIN32
#include <Rpc.h>
#else
#include <uuid/uuid.h>
#endif
}

#include "UUIDGenerator.h"
#include "Support/ErrorReporter.h"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Constants.h"
#include "llvm/Type.h"
#include "llvm/LLVMContext.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/Support/CallSite.h"
using namespace llvm;

char UUIDGenerator::ID = 0;
INITIALIZE_PASS(UUIDGenerator, "uuid-generator",
                "UUID generator pass", false, true)


bool UUIDGenerator::runOnModule(Module &M)
{
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
        runOnFunction(*I);
    return false;
}

void UUIDGenerator::runOnFunction(Function &F)
{
    for (Function::iterator I = F.begin(), E = F.end(); I != E; ++I)
        runOnBasicBlock(*I);
}

void UUIDGenerator::runOnBasicBlock (BasicBlock &BB)
{
    for (BasicBlock::iterator I = BB.begin(), E = BB.end(); I != E;) {
        CallSite CS(cast<Value>(I));
        ++I;

        if (CS) {
            Function *F = CS.getCalledFunction();
            if (F && F->getName() == "_SYS_lti_uuid")
                runOnCall(CS);
        }
    }
}

void UUIDGenerator::argUnpack(CallSite &CS, Args &a)
{
    Instruction *I = CS.getInstruction();

    if (CS.arg_size() != 2)
        report_fatal_error(I, "_SYS_lti_uuid requires two arguments");

    ConstantInt *CI = dyn_cast<ConstantInt>(CS.getArgument(0));
    if (!CI)
        report_fatal_error(I, "_SYS_lti_uuid key must be a constant integer");
    a.key = CI->getZExtValue();

    CI = dyn_cast<ConstantInt>(CS.getArgument(1));
    if (!CI)
        report_fatal_error(I, "_SYS_lti_uuid index must be a constant integer");
    a.index = CI->getZExtValue();
    if (a.index >= 4)
        report_fatal_error(I, "_SYS_lti_uuid index out of range");
}

void UUIDGenerator::runOnCall(CallSite &CS)
{
    Args a;
    argUnpack(CS, a);
    if (ValueMap.count(a.key) == 0)
        ValueMap[a.key] = generate();
}

uint32_t UUIDGenerator::getValueFor(CallSite &CS) const
{
    Args a;
    argUnpack(CS, a);
    ValueMap_t::const_iterator I = ValueMap.find(a.key);
    
    assert(I != ValueMap.end() && "Call site was never seen by UUIDGenerator");
    assert(a.index < 4);

    return I->second.words[a.index];
}

UUIDGenerator::UUID_t UUIDGenerator::generate()
{
#ifdef WIN32
    UUID uuid;
    UuidCreate ( &uuid );
#else
    uuid_t uuid;
    uuid_generate_random ( uuid );
#endif

    UUID_t result;
    assert(sizeof result == sizeof uuid);
    memcpy(&result, &uuid, sizeof result);
    printf("Generated one UUID: %08x %08x %08x %08x\n",
        result.words[0],result.words[1],result.words[2],result.words[3]);
    return result;
}
