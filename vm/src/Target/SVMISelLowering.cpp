/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMISelLowering.h"
#include "SVMTargetMachine.h"
#include "SVMMCTargetDesc.h"
#include "SVMMachineFunctionInfo.h"
#include "SVMSymbolDecoration.h"
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

#include "SVMGenCallingConv.inc"


SVMTargetLowering::SVMTargetLowering(SVMTargetMachine &TM)
    : TargetLowering(TM, new TargetLoweringObjectFileELF())
{
    // Standard library
    setLibcallName(RTLIB::MEMCPY, "_SYS_4");  // _SYS_memcpy8
    setLibcallName(RTLIB::MEMSET, "_SYS_1");  // _SYS_memset8

    // Software floating point library
    setLibcallName(RTLIB::ADD_F32, "_SYS_63");
    setLibcallName(RTLIB::ADD_F64, "_SYS_62");
    setLibcallName(RTLIB::SUB_F32, "_SYS_61");
    setLibcallName(RTLIB::SUB_F64, "_SYS_60");
    setLibcallName(RTLIB::MUL_F32, "_SYS_59");
    setLibcallName(RTLIB::MUL_F64, "_SYS_58");
    setLibcallName(RTLIB::DIV_F32, "_SYS_57");
    setLibcallName(RTLIB::DIV_F64, "_SYS_56");

    setLibcallName(RTLIB::FPEXT_F32_F64, "_SYS_55");
    setLibcallName(RTLIB::FPROUND_F64_F32, "_SYS_54");

    setLibcallName(RTLIB::FPTOSINT_F32_I8, "_SYS_53");
    setLibcallName(RTLIB::FPTOSINT_F32_I16, "_SYS_53");
    setLibcallName(RTLIB::FPTOSINT_F32_I32, "_SYS_53");
    setLibcallName(RTLIB::FPTOSINT_F32_I64, "_SYS_52");

    setLibcallName(RTLIB::FPTOSINT_F64_I8, "_SYS_51");
    setLibcallName(RTLIB::FPTOSINT_F64_I16, "_SYS_51");
    setLibcallName(RTLIB::FPTOSINT_F64_I32, "_SYS_51");
    setLibcallName(RTLIB::FPTOSINT_F64_I64, "_SYS_50");

    setLibcallName(RTLIB::FPTOUINT_F32_I8, "_SYS_49");
    setLibcallName(RTLIB::FPTOUINT_F32_I16, "_SYS_49");
    setLibcallName(RTLIB::FPTOUINT_F32_I32, "_SYS_49");
    setLibcallName(RTLIB::FPTOUINT_F32_I64, "_SYS_48");

    setLibcallName(RTLIB::FPTOUINT_F64_I8, "_SYS_47");
    setLibcallName(RTLIB::FPTOUINT_F64_I16, "_SYS_47");
    setLibcallName(RTLIB::FPTOUINT_F64_I32, "_SYS_47");
    setLibcallName(RTLIB::FPTOUINT_F64_I64, "_SYS_46");

    setLibcallName(RTLIB::SINTTOFP_I32_F32, "_SYS_45");
    setLibcallName(RTLIB::SINTTOFP_I32_F64, "_SYS_44");
    setLibcallName(RTLIB::SINTTOFP_I64_F32, "_SYS_43");
    setLibcallName(RTLIB::SINTTOFP_I64_F64, "_SYS_42");
    setLibcallName(RTLIB::UINTTOFP_I32_F32, "_SYS_41");
    setLibcallName(RTLIB::UINTTOFP_I32_F64, "_SYS_40");
    setLibcallName(RTLIB::UINTTOFP_I64_F32, "_SYS_39");
    setLibcallName(RTLIB::UINTTOFP_I64_F64, "_SYS_38");

    setLibcallName(RTLIB::OEQ_F32, "_SYS_37");
    setLibcallName(RTLIB::OEQ_F64, "_SYS_36");
    setLibcallName(RTLIB::UNE_F32, "_SYS_35");
    setLibcallName(RTLIB::UNE_F64, "_SYS_34");
    setLibcallName(RTLIB::OGE_F32, "_SYS_33");
    setLibcallName(RTLIB::OGE_F64, "_SYS_32");
    setLibcallName(RTLIB::OLT_F32, "_SYS_31");
    setLibcallName(RTLIB::OLT_F64, "_SYS_30");
    setLibcallName(RTLIB::OLE_F32, "_SYS_29");
    setLibcallName(RTLIB::OLE_F64, "_SYS_28");
    setLibcallName(RTLIB::OGT_F32, "_SYS_27");
    setLibcallName(RTLIB::OGT_F64, "_SYS_26");
    setLibcallName(RTLIB::UO_F32, "_SYS_25");
    setLibcallName(RTLIB::UO_F64, "_SYS_24");
    setLibcallName(RTLIB::O_F32, "_SYS_23");
    setLibcallName(RTLIB::O_F64, "_SYS_22");

    // Register classes
    addRegisterClass(MVT::i32, SVM::GPRegRegisterClass);
    addRegisterClass(MVT::i32, SVM::BPRegRegisterClass);
    addRegisterClass(MVT::i32, SVM::BPWriteRegRegisterClass);

    // Wrapped constants
    setOperationAction(ISD::GlobalAddress, MVT::i32, Custom);

    // Unsupported conditional operations
    setOperationAction(ISD::BRCOND, MVT::Other, Expand);
    setOperationAction(ISD::BRIND, MVT::Other, Expand);
    setOperationAction(ISD::BR_JT, MVT::Other, Expand);
    setOperationAction(ISD::SETCC, MVT::i32, Expand);
    setOperationAction(ISD::SELECT, MVT::i32, Expand);
    setOperationAction(ISD::SELECT_CC, MVT::i32, Custom);

    // Other unsupported operations
    setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i16, Expand);
    setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i8, Expand);
    setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i1, Expand);

    // Normal conditional branch
    setOperationAction(ISD::BR_CC, MVT::i32, Custom);

    // 64-bit operations
    setOperationAction(ISD::LOAD, MVT::i64, Expand);
    setOperationAction(ISD::STORE, MVT::i64, Expand);
    setOperationAction(ISD::ADD, MVT::i64, Expand);
    setOperationAction(ISD::SUB, MVT::i64, Expand);
    setOperationAction(ISD::SMUL_LOHI, MVT::i32, Expand);
    setOperationAction(ISD::UMUL_LOHI, MVT::i32, Expand);
    setOperationAction(ISD::MULHS, MVT::i32, Expand);
    setOperationAction(ISD::MULHU, MVT::i32, Expand);
    setOperationAction(ISD::SHL_PARTS, MVT::i32, Expand);
    setOperationAction(ISD::SRA_PARTS, MVT::i32, Expand);
    setOperationAction(ISD::SRL_PARTS, MVT::i32, Expand);

    computeRegisterProperties();
}

const char *SVMTargetLowering::getTargetNodeName(unsigned Opcode) const
{
    switch (Opcode) {
    default: return 0;
    case SVMISD::CALL:          return "SVMISD::CALL";
    case SVMISD::TAIL_CALL:     return "SVMISD::TAIL_CALL";
    case SVMISD::RETURN:        return "SVMISD::RETURN";
    case SVMISD::CMP:           return "SVMISD::CMP";
    case SVMISD::BRCOND:        return "SVMISD::BRCOND";
    case SVMISD::CMOV:          return "SVMISD::CMOV";
    case SVMISD::WRAPPER:       return "SVMISD::WRAPPER";
    case SVMISD::SYS64_CALL:    return "SVMISD::SYS64_CALL";
    }
}

SDValue SVMTargetLowering::LowerFormalArguments(SDValue Chain,
                                                CallingConv::ID CallConv,
                                                bool isVarArg,
                                                const SmallVectorImpl<ISD::InputArg> &Ins,
                                                DebugLoc dl, SelectionDAG &DAG,
                                                SmallVectorImpl<SDValue> &InVals) const
{
    MachineFunction &MF = DAG.getMachineFunction();
    
    // Let the calling convention assign locations to each operand
    SmallVector<CCValAssign, 16> ArgLocs;
    CCState CCInfo(CallConv, isVarArg, MF, DAG.getTarget(),
        ArgLocs, *DAG.getContext());
    CCInfo.AnalyzeFormalArguments(Ins, CC_SVM);

    for (unsigned i = 0, end = ArgLocs.size(); i != end; i++) {
        CCValAssign &VA = ArgLocs[i];

        if (VA.isRegLoc()) {
            // Passed by register. Convert the physical register to virtual.

            unsigned VReg = MF.getRegInfo().
                createVirtualRegister(SVM::GPRegRegisterClass);
            MF.getRegInfo().addLiveIn(VA.getLocReg(), VReg);
            SDValue ArgValue = DAG.getCopyFromReg(Chain, dl, VReg, MVT::i32);
            
            InVals.push_back(ArgValue);

        } else {
            llvm_unreachable("Non-register parameters not yet supported");
        }
    }
    
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
    // Let the calling convention assign locations to each operand
    SmallVector<CCValAssign, 16> ArgLocs;
    CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
        DAG.getTarget(), ArgLocs, *DAG.getContext());
    CCInfo.AnalyzeCallOperands(Outs, CC_SVM);
    
    // How much stack space do we require?
    unsigned ArgsStackSize = CCInfo.getNextStackOffset();

    // For now, assume 4-byte alignment
    ArgsStackSize = (ArgsStackSize + 3) & ~3;

    // Start calling sequence
    Chain = DAG.getCALLSEQ_START(Chain,
        DAG.getIntPtrConstant(ArgsStackSize, true));

    /*
     * Resolve address of callee, and detect syscalls.
     * This generates an opcode and argument list for our DAG node.
     */

    SVMDecorations Deco;
    Deco.isSys = false;
    if (GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee)) {
        const GlobalValue *GV = G->getGlobal();
        Deco.Decode(GV->getName());
        Callee = DAG.getTargetGlobalAddress(GV, dl, MVT::i32);        
    }

    unsigned Opcode;
    std::vector<SDValue> Ops;

    if (isTailCall) {
        // Generic tail-call
        Opcode = SVMISD::TAIL_CALL;
        Ops.push_back(Chain);
        Ops.push_back(Callee);
    
    } else if (Deco.isSys && Deco.sysNumber < 64) {
        // Inline syscall #0-63
        Opcode = SVMISD::SYS64_CALL;
        Ops.push_back(Chain);
        Ops.push_back(DAG.getConstant(Deco.sysNumber, MVT::i32));

    } else {
        // Generic call
        Opcode = SVMISD::CALL;
        Ops.push_back(Chain);
        Ops.push_back(Callee);
    }

    /*
     * Copy input parameters
     */

    SDValue InFlag;
    for (unsigned i = 0, end = ArgLocs.size(); i != end; i++) {
        CCValAssign &VA = ArgLocs[i];
        ISD::ArgFlagsTy Flags = Outs[i].Flags;
        SDValue Arg = OutVals[i];
        
        // Optional type promotion
        switch (VA.getLocInfo()) {
        default: llvm_unreachable("Unsupported parameter loc");
        case CCValAssign::Full: break;
        case CCValAssign::SExt:
            Arg = DAG.getNode(ISD::SIGN_EXTEND, dl, VA.getLocVT(), Arg);
            break;
        case CCValAssign::ZExt:
            Arg = DAG.getNode(ISD::ZERO_EXTEND, dl, VA.getLocVT(), Arg);
            break;
        case CCValAssign::AExt:
            Arg = DAG.getNode(ISD::ANY_EXTEND, dl, VA.getLocVT(), Arg);
            break;
        case CCValAssign::BCvt:
            Arg = DAG.getNode(ISD::BITCAST, dl, VA.getLocVT(), Arg);
            break;
        }
        
        if (VA.isRegLoc()) {
            // Passed by register
            
            unsigned Reg = VA.getLocReg();
            Chain = DAG.getCopyToReg(Chain, dl, Reg, Arg, InFlag);
            InFlag = Chain.getValue(1);

            // Reference this register, so it doesn't get optimized out
            Ops.push_back(DAG.getRegister(Reg, Arg.getValueType()));
            
        } else {
            llvm_unreachable("Non-register parameters not yet supported");
        }
    }
    
    if (InFlag.getNode())
      Ops.push_back(InFlag);

    /*
     * Emit the call DAG node
     */
    
    SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
    Chain = DAG.getNode(Opcode, dl, NodeTys, &Ops[0], Ops.size());
    InFlag = Chain.getValue(1);

    Chain = DAG.getCALLSEQ_END(Chain, DAG.getIntPtrConstant(0, true),
        DAG.getIntPtrConstant(0, true), InFlag);
    if (!Ins.empty())
        InFlag = Chain.getValue(1);

    return LowerCallResult(Chain, InFlag, CallConv, isVarArg, Ins,
                           dl, DAG, InVals);
}

SDValue SVMTargetLowering::LowerCallResult(SDValue Chain, SDValue InFlag,
    CallingConv::ID CallConv, bool isVarArg, const SmallVectorImpl<ISD::InputArg> &Ins,
    DebugLoc dl, SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const
{
    SmallVector<CCValAssign, 16> RVLocs;
    CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
        getTargetMachine(), RVLocs, *DAG.getContext());
    CCInfo.AnalyzeCallResult(Ins, RetCC_SVM);

    // Copy return values to vregs
    for (unsigned i = 0; i != RVLocs.size(); ++i) {
        CCValAssign VA = RVLocs[i];
        SDValue Val = DAG.getCopyFromReg(Chain, dl, VA.getLocReg(),
            VA.getLocVT(), InFlag);
        Chain = Val.getValue(1);
        InFlag = Val.getValue(2);

        // Value casting (type demotion)
        switch (VA.getLocInfo()) {
        default: llvm_unreachable("Unknown loc info!");
        case CCValAssign::Full: break;
        case CCValAssign::BCvt:
            Val = DAG.getNode(ISD::BITCAST, dl, VA.getValVT(), Val);
            break;
        }

        InVals.push_back(Val);
    }

    return Chain;
}

SDValue SVMTargetLowering::LowerReturn(SDValue Chain,
                                       CallingConv::ID CallConv, bool isVarArg,
                                       const SmallVectorImpl<ISD::OutputArg> &Outs,
                                       const SmallVectorImpl<SDValue> &OutVals,
                                       DebugLoc dl, SelectionDAG &DAG) const
{
    return DAG.getNode(SVMISD::RETURN, dl, MVT::Other, Chain);
}

SDValue SVMTargetLowering::LowerBR_CC(SDValue Op, SelectionDAG &DAG)
{
    /*
     * Trivial custom lowering for conditional branches; just do the
     * mapping from ISD branch conditions to SVMISD.
     */
    
    SDValue Chain = Op.getOperand(0);
    ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(1))->get();
    SDValue LHS = Op.getOperand(2);
    SDValue RHS = Op.getOperand(3);
    SDValue Dest = Op.getOperand(4);
    DebugLoc dl = Op.getDebugLoc();

    SDValue Cmp = DAG.getNode(SVMISD::CMP, dl, MVT::Glue, LHS, RHS);
    SDValue SVMcc = DAG.getConstant(SVMCC::mapTo(CC), MVT::i32);
    SDValue CCR = DAG.getRegister(SVM::CPSR, MVT::i32);

    return DAG.getNode(SVMISD::BRCOND, dl, MVT::Other,
        Chain, Dest, SVMcc, CCR, Cmp);
}

SDValue SVMTargetLowering::LowerSELECT_CC(SDValue Op, SelectionDAG &DAG)
{
    /*
     * We don't really support conditional moves, or any of LLVM's many
     * operations that are typically implemented in terms of conditional
     * moves. When we ask LegalizeDAG to expand all of these, they funnel
     * into SELECT_CC (LHS, RHS, TrueVal, FAlseVal, CC) -> Out.
     *
     * We'll need to replace this with branching, but we can't do that
     * during ISel. For now, insert a CMOV pseudo-op, which will be expanded
     * with a custom code emitter.
     *
     * (This is the same technique used by several other targets)
     */

    EVT VT = Op.getValueType();
    SDValue LHS = Op.getOperand(0);
    SDValue RHS = Op.getOperand(1);
    ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(4))->get();
    SDValue TrueVal = Op.getOperand(2);
    SDValue FalseVal = Op.getOperand(3);
    DebugLoc dl = Op.getDebugLoc();

    SDValue Cmp = DAG.getNode(SVMISD::CMP, dl, MVT::Glue, LHS, RHS);
    SDValue CCR = DAG.getRegister(SVM::CPSR, MVT::i32);
    
    return DAG.getNode(SVMISD::CMOV, dl, VT, TrueVal, FalseVal,
        DAG.getConstant(SVMCC::mapTo(CC), MVT::i32), CCR, Cmp);
}

SDValue SVMTargetLowering::LowerGlobalAddress(SDValue Op, SelectionDAG &DAG)
{
    DebugLoc dl = Op.getDebugLoc();
    EVT ValTy = Op.getValueType();
    GlobalAddressSDNode *SGA = cast<GlobalAddressSDNode>(Op);
    const GlobalValue *GV = SGA->getGlobal(); 
    int64_t Offset = SGA->getOffset();

    SDValue GA = DAG.getTargetGlobalAddress(GV, dl, ValTy, Offset);
    return DAG.getNode(SVMISD::WRAPPER, dl, ValTy, GA);
}

SDValue SVMTargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG) const
{
    switch (Op.getOpcode()) {
    default: llvm_unreachable("Should not custom lower this!");
    case ISD::BR_CC:         return LowerBR_CC(Op, DAG);
    case ISD::SELECT_CC:     return LowerSELECT_CC(Op, DAG);
    case ISD::GlobalAddress: return LowerGlobalAddress(Op, DAG);
    }
}
