/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVM.h"
#include "llvm/Module.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

Target llvm::TheSVMTarget;

extern "C" void LLVMInitializeSVMTargetInfo() { 
    RegisterTarget<Triple::thumb> X(TheSVMTarget, "svm", "SVM");
}
