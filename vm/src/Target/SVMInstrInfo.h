/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVMINSTRUCTIONINFO_H
#define SVMINSTRUCTIONINFO_H

#include "llvm/Target/TargetInstrInfo.h"
#include "SVMRegisterInfo.h"

#define GET_INSTRINFO_HEADER
#include "SVMGenInstrInfo.inc"

namespace llvm {

class SVMInstrInfo : public SVMGenInstrInfo {
public:
    explicit SVMInstrInfo(SVMSubtarget &ST);
    virtual const SVMRegisterInfo &getRegisterInfo() const { return RI; }
private:
    const SVMRegisterInfo RI;
};

}

#endif
