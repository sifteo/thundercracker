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
    class MCObjectWriter;
    class raw_ostream;
    
    MCObjectWriter *createSVMELFProgramWriter(raw_ostream &OS);
    FunctionPass *createSVMISelDag(SVMTargetMachine &TM);
    FunctionPass *createSVMAlignPass(SVMTargetMachine &TM);

} // namespace llvm

#endif
