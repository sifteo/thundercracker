/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_TARGETOBJECTFILE_H
#define SVM_TARGETOBJECTFILE_H

#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"

namespace llvm {

class SVMTargetObjectFile : public TargetLoweringObjectFileELF {
public:
    void Initialize(MCContext &Ctx, const TargetMachine &TM);
};

}

#endif
