/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_TARGETASMINFO_H
#define SVM_TARGETASMINFO_H

#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCAsmInfo.h"

namespace llvm {
    class Target;

    struct SVMMCAsmInfo : public MCAsmInfo {
        explicit SVMMCAsmInfo(const Target &T, StringRef TT);
    };

}

#endif
