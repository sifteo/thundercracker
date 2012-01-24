/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMISelLowering.h"
#include "SVMTargetMachine.h"
#include "SVMMachineFunctionInfo.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/ADT/VectorExtras.h"
#include "llvm/Support/ErrorHandling.h"
using namespace llvm;


SVMTargetLowering::SVMTargetLowering(TargetMachine &TM)
    : TargetLowering(TM, new TargetLoweringObjectFileELF())
{
    addRegisterClass(MVT::i32, SVM::IntRegsRegisterClass);
}

SDValue SVMTargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG) const
{
    switch (Op.getOpcode()) {
    default:
        llvm_unreachable("Should not custom lower this!");
    }
}

SDValue SVMTargetLowering::LowerFormalArguments(SDValue Chain,
                                                CallingConv::ID CallConv,
                                                bool isVarArg,
                                                const SmallVectorImpl<ISD::InputArg> &Ins,
                                                DebugLoc dl, SelectionDAG &DAG,
                                                SmallVectorImpl<SDValue> &InVals) const
{
    return Chain;
}

SDValue SVMTargetLowering::LowerCall(SDValue Chain, SDValue Callee,
                                     CallingConv::ID CallConv, bool isVarArg,
                                     bool &isTailCall,
                                     const SmallVectorImpl<ISD::OutputArg> &Outs,
                                     const SmallVectorImpl<SDValue> &OutVals,
                                     const SmallVectorImpl<ISD::InputArg> &Ins,
                                     DebugLoc dl, SelectionDAG &DAG,
                                     SmallVectorImpl<SDValue> &InVals) const
{
    return Chain;
}

SDValue SVMTargetLowering::LowerReturn(SDValue Chain,
                                       CallingConv::ID CallConv, bool isVarArg,
                                       const SmallVectorImpl<ISD::OutputArg> &Outs,
                                       const SmallVectorImpl<SDValue> &OutVals,
                                       DebugLoc dl, SelectionDAG &DAG) const
{
    return Chain;
}
