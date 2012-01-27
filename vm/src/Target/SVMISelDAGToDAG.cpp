/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMTargetMachine.h"
#include "SVMMCTargetDesc.h"
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
        
        // Complex patterns
        bool SelectAddrSP(SDValue N, SDValue &Base, SDValue &Offset);

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

bool SVMDAGToDAGISel::SelectAddrSP(SDValue Addr, SDValue &Base, SDValue &Offset)
{
    /*
     * Complex pattern to select valid stack addresses for LDRsp/STRsp.
     */

    EVT ValTy = Addr.getValueType();

    // Address itself is a FrameIndex
    if (FrameIndexSDNode *FIN = dyn_cast<FrameIndexSDNode>(Addr)) {
        Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), ValTy);
        Offset = CurDAG->getTargetConstant(0, ValTy);
        return true;
    }

    // FI+const or FI|const.
    // Note that OR is often used when the base FI has guaranteed alignment.
    if (CurDAG->isBaseWithConstantOffset(Addr)) {
        // Optional base portion, from the FI
        if (FrameIndexSDNode *FIN = dyn_cast<FrameIndexSDNode>(Addr.getOperand(0)))
            Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), ValTy);
        else
            Base = Addr.getOperand(0);

        // Offset portion
        ConstantSDNode *CN = dyn_cast<ConstantSDNode>(Addr.getOperand(1));
        Offset = CurDAG->getTargetConstant(CN->getZExtValue(), ValTy);
        return true;
    }

    // No transformation
    Base = Addr;
    Offset = CurDAG->getTargetConstant(0, ValTy);
    return true;
}

FunctionPass *llvm::createSVMISelDag(SVMTargetMachine &TM) {
    return new SVMDAGToDAGISel(TM);
}
