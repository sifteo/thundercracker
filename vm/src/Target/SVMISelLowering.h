/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo VM (SVM) Target for LLVM
 *
 * Micah Elizabeth Scott <micah@misc.name>
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
            TAIL_CALL,
            RETURN,
            CMP,
            BRCOND,
            CMOV,
            WRAPPER,
            SYS64_CALL,
        };
    }
    
    class SVMTargetMachine;
    class CCValAssign;

    class SVMTargetLowering : public TargetLowering {
    public:
        SVMTargetLowering(SVMTargetMachine &TM);

        virtual const char *getTargetNodeName(unsigned Opcode) const;

        virtual MachineBasicBlock *
          EmitInstrWithCustomInserter(MachineInstr *MI,
                                      MachineBasicBlock *BB) const;

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
          LowerCallResult(SDValue Chain, SDValue Glue,
                          CallingConv::ID CallConv, bool isVarArg,
                          const SmallVectorImpl<ISD::InputArg> &Ins,
                          DebugLoc dl, SelectionDAG &DAG,
                          SmallVectorImpl<SDValue> &InVals) const;

        virtual SDValue
          LowerReturn(SDValue Chain,
                      CallingConv::ID CallConv, bool isVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals,
                      DebugLoc dl, SelectionDAG &DAG) const;
                      
    private:
        // Custom lowering
        static SDValue LowerBR_CC(SDValue Op, SelectionDAG &DAG);
        static SDValue LowerSELECT_CC(SDValue Op, SelectionDAG &DAG);
        static SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG);
        static SDValue LowerDYNAMIC_STACKALLOC(SDValue Op, SelectionDAG &DAG);

        // Custom inserters
        MachineBasicBlock *ExpandCMOV(MachineInstr *MI, MachineBasicBlock *BB) const;

        static SDValue promoteArg(DebugLoc dl, SelectionDAG &DAG, CCValAssign &VA, SDValue Arg);
        static unsigned resolveCallOpcode(DebugLoc dl, SelectionDAG &DAG,
            SDValue &Callee, bool isTailCall);
    };
}

#endif
