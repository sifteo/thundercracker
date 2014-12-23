/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo VM (SVM) Target for LLVM
 *
 * Micah Elizabeth Scott <micah@misc.name>
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
      DataLayout(getDataLayoutString()),
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

uint32_t SVMTargetMachine::getRAMSize()
{
    return 0x8000;
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

const char *SVMTargetMachine::getDataLayoutString()
{
    return "e-S32-p32:32:32-i64:32:32-f64:32:32-v64:32:32-a0:1:1-s0:32:32-n32";
}

bool SVMTargetMachine::isTargetCompatible(LLVMContext& Context, const TargetData &TD)
{
    Type *i32 = Type::getInt64Ty(Context);
    Type *i64 = Type::getInt64Ty(Context);
    Type *i32i64i32 = StructType::get(i32, i64, i32, NULL);

    return
        TD.getPointerABIAlignment() == 4 &&
        TD.getPointerSize() == 4 &&
        TD.exceedsNaturalStackAlignment(4) == false &&
        TD.exceedsNaturalStackAlignment(8) == true &&
        TD.getABITypeAlignment(i64) == 4 &&
        TD.getABITypeAlignment(i32i64i32) == 4 &&
        TD.getTypeAllocSize(i32i64i32) == 24;
}
