/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_ISELLOWERING_H
#define SVM_ISELLOWERING_H

#include "llvm/Target/TargetLowering.h"
#include "SVM.h"

namespace llvm {

    class SVMTargetLowering : public TargetLowering {
    public:
        SVMTargetLowering(TargetMachine &TM);
    };

}

#endif
