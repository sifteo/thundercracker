/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMTargetMachine.h"
#include "SVMMCTargetDesc.h"
#include "SVMSymbolDecoration.h"
#include "llvm/Intrinsics.h"
#include "llvm/GlobalValue.h"
#include "llvm/GlobalAlias.h"
#include "llvm/Module.h"
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
            : SelectionDAGISel(tm), TM(tm), M(0) {}

        SDNode *Select(SDNode *N);

        virtual const char *getPassName() const {
            return "SVM DAG->DAG Pattern Instruction Selection";
        }

        bool doInitialization(Module &Mod) {
            M = &Mod;
            return false;
        }
        
        // Complex patterns
        bool SelectAddrSP(SDValue Addr, SDValue &Base, SDValue &Offset);
        bool SelectCallTarget(SDValue Addr, SDValue &CP);
        bool SelectTailCallTarget(SDValue Addr, SDValue &CP);
        bool SelectLDAddrTarget(SDValue Addr, SDValue &CP);

        #include "SVMGenDAGISel.inc"
        
    private:
        Module *M;
        
        bool SelectDecoratedTarget(SDValue Addr, SDValue &CP, const char *Prefix);
        bool shouldUseConstantPool(uint32_t val);
        void moveConstantToPool(SDNode *N, uint32_t val);
        
        ConstantInt *const32(uint32_t val) {
            return ConstantInt::get(
                Type::getInt32Ty(*CurDAG->getContext()), val);
        }
    };
}

SDNode *SVMDAGToDAGISel::Select(SDNode *N)
{
    DebugLoc dl = N->getDebugLoc();

    if (N->isMachineOpcode())
        return NULL;

    switch (N->getOpcode()) {
    default:
        break;

    // Convert inline constants to constpool entries when needed
    case ISD::Constant: {
        uint32_t val = cast<ConstantSDNode>(N)->getZExtValue();
        if (shouldUseConstantPool(val)) {
            moveConstantToPool(N, val);
            return NULL;
        }
        break;
    }

    /*
     * Pattern (SVMBrcond bb:$offset8, imm:$cc)
     * Emits (Bcc bccTarget:$offset8, CCop:$cc)
     * We must do this programmatically so we can glue the CMP instruction properly.
     */
    case SVMISD::BRCOND: {
        SDValue Chain = N->getOperand(0);
        SDValue N1 = N->getOperand(1);
        SDValue N2 = N->getOperand(2);
        SDValue N3 = N->getOperand(3);
        SDValue InGlue = N->getOperand(4);
        assert(N1.getOpcode() == ISD::BasicBlock);  // Target
        assert(N2.getOpcode() == ISD::Constant);    // CC
        assert(N3.getOpcode() == ISD::Register);    // CPSR
        
        // Make a new condition code constant
        SDValue CCval = CurDAG->getTargetConstant(((unsigned)
            cast<ConstantSDNode>(N2)->getZExtValue()), MVT::i32);

        SDValue Ops[] = { N1, CCval, N3, Chain, InGlue };
        SDNode *R = CurDAG->getMachineNode(SVM::Bcc, dl, MVT::Other,
            MVT::Glue, Ops, 5);
        Chain = SDValue(R, 0);
        InGlue = SDValue(R, 1);

        if (N->getNumValues() == 2)
            ReplaceUses(SDValue(N, 1), InGlue);
        ReplaceUses(SDValue(N, 0), Chain);
        return NULL;
    }
    }

    return SelectCode(N);
}

bool SVMDAGToDAGISel::shouldUseConstantPool(uint32_t val)
{
    /*
     * Is this value more efficient or only possible to express using a
     * constant pool?
     */
    return val >= 0x10000;
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
        if (FrameIndexSDNode *FIN = dyn_cast<FrameIndexSDNode>(Addr.getOperand(0))) {
            ConstantSDNode *CN = dyn_cast<ConstantSDNode>(Addr.getOperand(1));
            Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), ValTy);
            Offset = CurDAG->getTargetConstant(CN->getZExtValue(), ValTy);
            return true;
        }
    }

    return false;
}

bool SVMDAGToDAGISel::SelectDecoratedTarget(SDValue Addr,
    SDValue &CP, const char *Prefix)
{
    // Decorated address, no offset support
    if (GlobalAddressSDNode *GA = dyn_cast<GlobalAddressSDNode>(Addr)) {
        assert(GA->getOffset() == 0);
        CP = CurDAG->getTargetConstantPool(
            SVMDecorations::Apply(M, GA->getGlobal(), Prefix), MVT::i32);
        return true;
    }
    
    // Wrap ExternalSymbols in a GlobalValue, so we can uniquely alias them
    if (ExternalSymbolSDNode *ES = dyn_cast<ExternalSymbolSDNode>(Addr)) {
        Type *T = FunctionType::get(Type::getVoidTy(*CurDAG->getContext()), false);
        GlobalValue *GV = dyn_cast<GlobalValue>
            (M->getOrInsertGlobal(ES->getSymbol(), T));            
        CP = CurDAG->getTargetConstantPool(
            SVMDecorations::Apply(M, GV, Prefix), MVT::i32);
        return true;
    }

    return false;
}

bool SVMDAGToDAGISel::SelectCallTarget(SDValue Addr, SDValue &CP)
{
    return SelectDecoratedTarget(Addr, CP, SVMDecorations::CALL);
}

bool SVMDAGToDAGISel::SelectTailCallTarget(SDValue Addr, SDValue &CP)
{
    return SelectDecoratedTarget(Addr, CP, SVMDecorations::TCALL);
}

bool SVMDAGToDAGISel::SelectLDAddrTarget(SDValue Addr, SDValue &CP)
{
    if (GlobalAddressSDNode *GA = dyn_cast<GlobalAddressSDNode>(Addr)) {
        /*
         * Fold the GlobalValue into a constant pool entry.
         *
         * XXX: We should prefer to keep the offset in the load/store imm12
         *      instead of baking it into a constant pool entry! How do we
         *      do that?
         */
        CP = CurDAG->getTargetConstantPool(
            SVMDecorations::ApplyOffset(M, GA->getGlobal(), GA->getOffset()),
            MVT::i32);
        return true;
    }
    return false;
}

FunctionPass *llvm::createSVMISelDag(SVMTargetMachine &TM) {
    return new SVMDAGToDAGISel(TM);
}
