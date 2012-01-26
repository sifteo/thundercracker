/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_ISELLOWERING_H
#define SVM_ISELLOWERING_H

#include "llvm/Target/TargetLowering.h"
#include "SVM.h"

namespace llvm {

    namespace SVMISD {
        // SVM specific DAG nodes
        enum NodeType {
            FIRST_NUMBER = ISD::BUILTIN_OP_END,
            CALL,
            RETURN,
            CMP,
            BRCOND,
        };
    }

    class SVMTargetLowering : public TargetLowering {
    public:
        SVMTargetLowering(TargetMachine &TM);

        virtual const char *getTargetNodeName(unsigned Opcode) const;

        virtual SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const;

        virtual SDValue
          LowerFormalArguments(SDValue Chain,
                               CallingConv::ID CallConv,
                               bool isVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               DebugLoc dl, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const;

        virtual SDValue
          LowerCall(SDValue Chain, SDValue Callee,
                    CallingConv::ID CallConv, bool isVarArg,
                    bool &isTailCall,
                    const SmallVectorImpl<ISD::OutputArg> &Outs,
                    const SmallVectorImpl<SDValue> &OutVals,
                    const SmallVectorImpl<ISD::InputArg> &Ins,
                    DebugLoc dl, SelectionDAG &DAG,
                    SmallVectorImpl<SDValue> &InVals) const;

        virtual SDValue
          LowerReturn(SDValue Chain,
                      CallingConv::ID CallConv, bool isVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals,
                      DebugLoc dl, SelectionDAG &DAG) const;
    };
}

#endif
