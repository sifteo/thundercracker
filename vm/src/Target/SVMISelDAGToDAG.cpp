/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMTargetMachine.h"
#include "llvm/Intrinsics.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

namespace {
    class SVMDAGToDAGISel : public SelectionDAGISel {
        SVMTargetMachine& TM;
    public:
        explicit SVMDAGToDAGISel(SVMTargetMachine &tm)
            : SelectionDAGISel(tm), TM(tm) {}

        SDNode *Select(SDNode *N);

        virtual const char *getPassName() const {
            return "SVM DAG->DAG Pattern Instruction Selection";
        }

        #include "SVMGenDAGISel.inc"
    };
}

SDNode *SVMDAGToDAGISel::Select(SDNode *N)
{
    if (N->isMachineOpcode())
        return NULL;

    switch (N->getOpcode()) {
    default:
        break;
    }

    return SelectCode(N);
}
