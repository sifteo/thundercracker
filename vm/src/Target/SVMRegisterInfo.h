/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_REGISTERINFO_H
#define SVM_REGISTERINFO_H

#include "llvm/Target/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "SVMGenRegisterInfo.inc"

namespace llvm {

class TargetInstrInfo;
class Type;

struct SVMRegisterInfo : public SVMGenRegisterInfo {
    const TargetInstrInfo &TII;

    SVMRegisterInfo(const TargetInstrInfo &tii);
};

} // end namespace llvm

#endif
