/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_MCTARGETDESC_H
#define SVM_MCTARGETDESC_H

namespace llvm {
    class MCSubtargetInfo;
    class Target;
    class StringRef;

    extern Target TheSVMTarget;
}

#define GET_REGINFO_ENUM
#include "SVMGenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#include "SVMGenInstrInfo.inc"

#endif
