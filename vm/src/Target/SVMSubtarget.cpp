/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMSubtarget.h"
#include "SVM.h"
#include "llvm/Support/TargetRegistry.h"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "SVMGenSubtargetInfo.inc"

using namespace llvm;

SVMSubtarget::SVMSubtarget(const std::string &TT, const std::string &CPU, const std::string &FS)
    : SVMGenSubtargetInfo(TT, CPU, FS)
{
}
