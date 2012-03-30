/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_ERRORREPORTER_H
#define SVM_ERRORREPORTER_H

namespace llvm {

    class Instruction;
    class Twine;

    void report_fatal_error(const Instruction *I, const Twine &msg);
    void report_fatal_error(const Instruction *I, const char *msg);
    
} // end namespace

#endif
