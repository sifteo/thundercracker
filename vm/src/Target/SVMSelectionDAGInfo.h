/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_SELECTIONDAGINFO_H
#define SVM_SELECTIONDAGINFO_H

#include "llvm/Target/TargetSelectionDAGInfo.h"

namespace llvm {

class SVMTargetMachine;

class SVMSelectionDAGInfo : public TargetSelectionDAGInfo {
public:
    explicit SVMSelectionDAGInfo(const SVMTargetMachine &TM);
    ~SVMSelectionDAGInfo();
};

}

#endif
