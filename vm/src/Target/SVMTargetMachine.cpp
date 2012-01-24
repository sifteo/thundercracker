/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVM.h"
#include "SVMMCTargetDesc.h"
#include "SVMTargetMachine.h"
#include "llvm/PassManager.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

extern "C" void LLVMInitializeSVMTarget()
{
    RegisterTargetMachine<SVMTargetMachine> X(TheSVMTarget);
}

SVMTargetMachine::SVMTargetMachine(const Target &T, StringRef TT, 
                                   StringRef CPU, StringRef FS,
                                   Reloc::Model RM, CodeModel::Model CM)
    : LLVMTargetMachine(T, TT, CPU, FS, RM, CM),
      DataLayout("E-p:32:32:32-i64:64:64-f64:64:64-f128:64:64-n32"),
      TLInfo(*this), TSInfo(*this), Subtarget(TT, CPU, FS)
{}

bool SVMTargetMachine::addInstSelector(PassManagerBase &PM, CodeGenOpt::Level OptLevel)
{
    PM.add(createSVMISelDag(*this));
    return false;
}
