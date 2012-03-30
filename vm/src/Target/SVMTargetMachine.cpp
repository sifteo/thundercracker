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
      DataLayout("e-S32-p32:32:32-i64:32:32-f64:32:32-v64:32:32-a0:1:1-s0:32:32-n32"),
      TLInfo(*this), TSInfo(*this), Subtarget(TT, CPU, FS)
{}

bool SVMTargetMachine::addInstSelector(PassManagerBase &PM, CodeGenOpt::Level OptLevel)
{
    PM.add(createSVMISelDag(*this));
    return false;
}

bool SVMTargetMachine::addPreEmitPass(PassManagerBase &PM, CodeGenOpt::Level OptLevel)
{
    // The Alignment pass may change the size of functions by inserting no-ops,
    // so it must come before the LateFunctionSplitPass.
    PM.add(createSVMAlignPass(*this));
    
    // Last-resort function splitting. Must come after AlignPass.
    PM.add(createSVMLateFunctionSplitPass(*this));
    
    return true;
}

uint32_t SVMTargetMachine::getBlockSize()
{
    return 256;
}

uint32_t SVMTargetMachine::getBundleSize()
{
    return 4;
}

uint32_t SVMTargetMachine::getFlashBase()
{
    return 0x80000000;
}

uint32_t SVMTargetMachine::getRAMBase()
{
    return 0x10000;
}

uint8_t SVMTargetMachine::getPaddingByte()
{
    /*
     * Instead of padding with zeroes, pad with ones.
     * These programs get stored in flash memory, and 0xFF
     * is what the flash erases to.
     */
    return 0xFF;
}
