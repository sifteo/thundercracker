/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef TARGET_SVM_H
#define TARGET_SVM_H

namespace llvm {

    class FunctionPass;
    class SVMTargetMachine;

    FunctionPass *createSVMISelDag(SVMTargetMachine &TM);

} // namespace llvm

#endif
