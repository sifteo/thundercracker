/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVM.h"
#include "SVMRegisterInfo.h"

#define GET_REGINFO_TARGET_DESC
#include "SVMGenRegisterInfo.inc"

using namespace llvm;

SVMRegisterInfo::SVMRegisterInfo(const TargetInstrInfo &tii)
    : TII(tii) {}
