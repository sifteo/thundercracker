/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_SUBTARGET_H
#define SVM_SUBTARGET_H

#include "llvm/Target/TargetSubtargetInfo.h"
#include "llvm/Target/TargetMachine.h"
#include <string>

#define GET_SUBTARGETINFO_HEADER
#include "SVMGenSubtargetInfo.inc"

namespace llvm {
    class StringRef;

    class SVMSubtarget : public SVMGenSubtargetInfo {
    public:
        SVMSubtarget(const std::string &TT, const std::string &CPU, const std::string &FS);
        void ParseSubtargetFeatures(StringRef CPU, StringRef FS);
    };
}

#endif
