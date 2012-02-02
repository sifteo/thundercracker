/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMTargetMachine.h"
#include "SVMMCTargetDesc.h"
#include "llvm/Intrinsics.h"
#include "llvm/GlobalValue.h"
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
        
        // Custom selectors
        bool shouldUseConstantPool(uint32_t val);
        void moveConstantToPool(SDNode *N, uint32_t val);
        
        // Complex patterns
        bool SelectAddrSP(SDValue Addr, SDValue &Base, SDValue &Offset);
        bool SelectCallTarget(SDValue Addr, SDValue &CP);
        bool SelectTailCallTarget(SDValue Addr, SDValue &CP);

        #include "SVMGenDAGISel.inc"
        
    private:
        ConstantInt *const32(uint32_t val) {
            return ConstantInt::get(
                Type::getInt32Ty(*CurDAG->getContext()), val);
        }
    };
}

SDNode *SVMDAGToDAGISel::Select(SDNode *N)
{
    if (N->isMachineOpcode())
        return NULL;

    switch (N->getOpcode()) {

    case ISD::Constant: {
        uint32_t val = cast<ConstantSDNode>(N)->getZExtValue();
        if (shouldUseConstantPool(val)) {
            moveConstantToPool(N, val);
            return NULL;
        }
        break;
    }

    default:
        break;
    }

    return SelectCode(N);
}

bool SVMDAGToDAGISel::shouldUseConstantPool(uint32_t val)
{
    /*
     * Is this value more efficient or only possible to express using a
     * constant pool?
     *
     * Right now we only consider the range of MOVSi8. All constants
     * over 255 are moved to the pool. In the future it may make more sense
     * to generate some values programmatically using negation or shifting,
     * if this can be done using fewer unshared bytes than a const pool entry.
     */
    return val > 0xFF;
}

void SVMDAGToDAGISel::moveConstantToPool(SDNode *N, uint32_t val)
{
    /*
     * Convert a Constant node to a constant pool entry. We do this
     * automatically for constants that are too large for a MOVSi8.
     */
    
    SDValue CPIdx = CurDAG->getTargetConstantPool(const32(val), TLI.getPointerTy());
    DebugLoc dl = N->getDebugLoc();
    SDNode *newNode = CurDAG->getMachineNode(SVM::LDRpc, dl, MVT::i32,
                                             MVT::Other, CPIdx,
                                             CurDAG->getEntryNode());

    ReplaceUses(SDValue(N, 0), SDValue(newNode, 0));
}

bool SVMDAGToDAGISel::SelectAddrSP(SDValue Addr, SDValue &Base, SDValue &Offset)
{
    // Complex pattern to select valid stack addresses for LDRsp/STRsp.

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

bool SVMDAGToDAGISel::SelectCallTarget(SDValue Addr, SDValue &CP)
{
    if (GlobalAddressSDNode *GA = dyn_cast<GlobalAddressSDNode>(Addr)) {
        CP = CurDAG->getTargetConstantPool(
            GA->getGlobal(), MVT::i32, 0, 0, SVMTOF::CALL);
        return true;
    }
    return false;
}

bool SVMDAGToDAGISel::SelectTailCallTarget(SDValue Addr, SDValue &CP)
{
    if (GlobalAddressSDNode *GA = dyn_cast<GlobalAddressSDNode>(Addr)) {
        CP = CurDAG->getTargetConstantPool(
            GA->getGlobal(), MVT::i32, 0, 0, SVMTOF::TAIL_CALL);
        return true;
    }
    return false;
}

FunctionPass *llvm::createSVMISelDag(SVMTargetMachine &TM) {
    return new SVMDAGToDAGISel(TM);
}
