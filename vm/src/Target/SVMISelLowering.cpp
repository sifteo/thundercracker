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

    setLibcallName(RTLIB::OEQ_F32, "_SYS_37");  // _SYS_eq_f32
    setCmpLibcallCC(RTLIB::OEQ_F32, ISD::SETNE);
    setLibcallName(RTLIB::UNE_F32, "_SYS_37");  // _SYS_eq_f32
    setCmpLibcallCC(RTLIB::UNE_F32, ISD::SETEQ);
    setLibcallName(RTLIB::OLT_F32, "_SYS_35");  // _SYS_lt_f32
    setCmpLibcallCC(RTLIB::OLT_F32, ISD::SETNE);
    setLibcallName(RTLIB::OLE_F32, "_SYS_33");  // _SYS_le_f32
    setCmpLibcallCC(RTLIB::OLE_F32, ISD::SETNE);
    setLibcallName(RTLIB::OGE_F32, "_SYS_31");  // _SYS_ge_f32
    setCmpLibcallCC(RTLIB::OGE_F32, ISD::SETNE);
    setLibcallName(RTLIB::OGT_F32, "_SYS_29");  // _SYS_gt_f32
    setCmpLibcallCC(RTLIB::OGT_F32, ISD::SETNE);
    setLibcallName(RTLIB::UO_F32,  "_SYS_27");  // _SYS_un_f32
    setCmpLibcallCC(RTLIB::UO_F32,  ISD::SETNE);
    setLibcallName(RTLIB::O_F32,   "_SYS_27");  // _SYS_un_f32
    setCmpLibcallCC(RTLIB::O_F32,   ISD::SETEQ);

    setLibcallName(RTLIB::OEQ_F64, "_SYS_36");  // _SYS_eq_f64
    setCmpLibcallCC(RTLIB::OEQ_F64, ISD::SETNE);
    setLibcallName(RTLIB::UNE_F64, "_SYS_36");  // _SYS_eq_f64
    setCmpLibcallCC(RTLIB::UNE_F64, ISD::SETEQ);
    setLibcallName(RTLIB::OLT_F64, "_SYS_34");  // _SYS_lt_f64
    setCmpLibcallCC(RTLIB::OLT_F64, ISD::SETNE);
    setLibcallName(RTLIB::OLE_F64, "_SYS_32");  // _SYS_le_f64
    setCmpLibcallCC(RTLIB::OLE_F64, ISD::SETNE);
    setLibcallName(RTLIB::OGE_F64, "_SYS_30");  // _SYS_ge_f64
    setCmpLibcallCC(RTLIB::OGE_F64, ISD::SETNE);
    setLibcallName(RTLIB::OGT_F64, "_SYS_28");  // _SYS_gt_f64
    setCmpLibcallCC(RTLIB::OGT_F64, ISD::SETNE);
    setLibcallName(RTLIB::UO_F64,  "_SYS_26");  // _SYS_un_f64
    setCmpLibcallCC(RTLIB::UO_F64,  ISD::SETNE);
    setLibcallName(RTLIB::O_F64,   "_SYS_26");  // _SYS_un_f64
    setCmpLibcallCC(RTLIB::O_F64,   ISD::SETEQ);

    // Atomic operations
    setLibcallName(RTLIB::SYNC_FETCH_AND_OR_4, "_SYS_110");
    setLibcallName(RTLIB::SYNC_FETCH_AND_XOR_4, "_SYS_111");
    setLibcallName(RTLIB::SYNC_FETCH_AND_NAND_4, "_SYS_112");
    setLibcallName(RTLIB::SYNC_FETCH_AND_AND_4, "_SYS_113");

    // 64-bit Integer operations
    setLibcallName(RTLIB::SHL_I64, "_SYS_17");
    setLibcallName(RTLIB::SRL_I64, "_SYS_18");
    setLibcallName(RTLIB::SRA_I64, "_SYS_19");
    setLibcallName(RTLIB::MUL_I64, "_SYS_20");
    setLibcallName(RTLIB::SDIV_I64, "_SYS_21");
    setLibcallName(RTLIB::UDIV_I64, "_SYS_22");
    setLibcallName(RTLIB::SREM_I64, "_SYS_23");
    setLibcallName(RTLIB::UREM_I64, "_SYS_24");

    // Register classes to allocate into
    addRegisterClass(MVT::i32, SVM::GPRegRegisterClass);

    // Wrapped constants
    setOperationAction(ISD::GlobalAddress, MVT::i32, Custom);

    // Unsupported conditional operations
    setOperationAction(ISD::BRCOND, MVT::Other, Expand);
    setOperationAction(ISD::BRIND, MVT::Other, Expand);
    setOperationAction(ISD::BR_JT, MVT::Other, Expand);
    setOperationAction(ISD::SETCC, MVT::i32, Expand);
    setOperationAction(ISD::SELECT, MVT::i32, Expand);
    setOperationAction(ISD::SELECT_CC, MVT::i32, Custom);

    // Load/store operations
    setLoadExtAction(ISD::SEXTLOAD, MVT::i1, Promote);
    setLoadExtAction(ISD::ZEXTLOAD, MVT::i1, Promote);
    setLoadExtAction(ISD::EXTLOAD, MVT::i1, Promote);

    // Other unsupported operations
    setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i16, Expand);
    setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i8, Expand);
    setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i1, Expand);
    setOperationAction(ISD::SREM, MVT::i32, Expand);
    setOperationAction(ISD::UREM, MVT::i32, Expand);
    setOperationAction(ISD::SDIVREM, MVT::i32, Expand);
    setOperationAction(ISD::UDIVREM, MVT::i32, Expand);
    setOperationAction(ISD::CTPOP, MVT::i32, Expand);

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

    // Atomics
    setOperationAction(ISD::MEMBARRIER, MVT::Other, Expand);
    setOperationAction(ISD::ATOMIC_FENCE, MVT::Other, Expand);
    setOperationAction(ISD::ATOMIC_CMP_SWAP, MVT::i32, Expand);
    setOperationAction(ISD::ATOMIC_SWAP, MVT::i32, Expand);
    setOperationAction(ISD::ATOMIC_LOAD_ADD, MVT::i32, Expand);
    setOperationAction(ISD::ATOMIC_LOAD_SUB, MVT::i32, Expand);
    setOperationAction(ISD::ATOMIC_LOAD_AND, MVT::i32, Expand);
    setOperationAction(ISD::ATOMIC_LOAD_OR, MVT::i32, Expand);
    setOperationAction(ISD::ATOMIC_LOAD_XOR, MVT::i32, Expand);
    setOperationAction(ISD::ATOMIC_LOAD_NAND, MVT::i32, Expand);
    setOperationAction(ISD::ATOMIC_LOAD_MIN, MVT::i32, Expand);
    setOperationAction(ISD::ATOMIC_LOAD_MAX, MVT::i32, Expand);
    setOperationAction(ISD::ATOMIC_LOAD_UMIN, MVT::i32, Expand);
    setOperationAction(ISD::ATOMIC_LOAD_UMAX, MVT::i32, Expand);
    setOperationAction(ISD::ATOMIC_LOAD, MVT::i32, Expand);
    setOperationAction(ISD::ATOMIC_STORE, MVT::i32, Expand);
    setShouldFoldAtomicFences(true);

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
    MachineFrameInfo *MFI = MF.getFrameInfo();
    
    // Let the calling convention assign locations to each operand
    SmallVector<CCValAssign, 16> ArgLocs;
    CCState CCInfo(CallConv, isVarArg, MF, DAG.getTarget(),
        ArgLocs, *DAG.getContext());
    CCInfo.AnalyzeFormalArguments(Ins, CC_SVM);

    for (unsigned i = 0, end = ArgLocs.size(); i != end; i++) {
        CCValAssign &VA = ArgLocs[i];
        assert(VA.isMemLoc() || VA.isRegLoc());

        if (VA.isRegLoc()) {
            // Passed by register. Convert the physical register to virtual.

            unsigned VReg = MF.getRegInfo().
                createVirtualRegister(SVM::GPRegRegisterClass);
            MF.getRegInfo().addLiveIn(VA.getLocReg(), VReg);
            SDValue ArgValue = DAG.getCopyFromReg(Chain, dl, VReg, MVT::i32);
            
            InVals.push_back(ArgValue);

        } else {
            // Passed on the stack. We need to reach up into the caller's
            // stack frame to retrieve this value. We don't know the offset
            // for the caller's frame at this point; that needs to be resolved
            // by SVMRegisterInfo::eliminateFrameIndex.
            //
            // At this stage in compilation, we represent offsets into the
            // caller's frame with negative fixed object offsets. -1 corresponds
            // to offset 0 in the caller's frame, -5 corresponds to offset 4,
            // and so on.

            // Allocate a fixed stack object for this parameter
            unsigned Size = VA.getLocVT().getSizeInBits() / 8;
            unsigned Offset = VA.getLocMemOffset();
            int FI = MFI->CreateFixedObject(Size, -(1 + Offset), true);
            assert(Size == 4 && "Expected params to be converted to i32");

            // Load from this stack object
            SDValue FIN = DAG.getFrameIndex(FI, MVT::i32);
            SDValue Load = DAG.getLoad(MVT::i32, dl, Chain, FIN,
                MachinePointerInfo::getFixedStack(FI), false, false, 0);

            InVals.push_back(Load);
        }
    }
    
    return Chain;
}

SDValue SVMTargetLowering::promoteArg(DebugLoc dl, SelectionDAG &DAG,
    CCValAssign &VA, SDValue Arg)
{
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
    return Arg;
}

unsigned SVMTargetLowering::resolveCallOpcode(DebugLoc dl, SelectionDAG &DAG,
    SDValue &Callee, bool isTailCall)
{
    /*
     * Resolve address of callee, and detect syscalls.
     * This generates an opcode and argument list for our DAG node.
     */

    SVMDecorations Deco;

    if (GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee)) {
        const GlobalValue *GV = G->getGlobal();
        Deco.Decode(GV->getName());
        Callee = DAG.getTargetGlobalAddress(GV, dl, MVT::i32);        
    } else {
        Deco.Init();
    }

    if (isTailCall) {
        // Generic tail-call
        return SVMISD::TAIL_CALL;
    }
    
    if (Deco.isSys && Deco.sysNumber < 64) {
        // Inline syscall #0-63
        Callee = DAG.getConstant(Deco.sysNumber, MVT::i32);
        return SVMISD::SYS64_CALL;
    }
    
    // Generic call
    return SVMISD::CALL;
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
    MachineFunction &MF = DAG.getMachineFunction();
    MachineFrameInfo *MFI = MF.getFrameInfo();

    // Let the calling convention assign locations to each operand
    SmallVector<CCValAssign, 16> ArgLocs;
    CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
        DAG.getTarget(), ArgLocs, *DAG.getContext());
    CCInfo.AnalyzeCallOperands(Outs, CC_SVM);

    /*
     * Copy input parameters to the stack. Do this, in full, before copying
     * anything to registers.
     *
     * We manage the Chains here such that all stores can happen in any order,
     * but they're all guaranteed to finish before we start assigning the
     * register inputs.
     */

    SmallVector<SDValue, 8> MemOpChains;

    for (unsigned i = 0, end = ArgLocs.size(); i != end; i++) {
        CCValAssign &VA = ArgLocs[i];
        assert(VA.isMemLoc() || VA.isRegLoc());
        
        if (VA.isMemLoc()) {
            SDValue Arg = promoteArg(dl, DAG, VA, OutVals[i]);

            // Allocate a fixed stack object for this parameter
            unsigned Size = VA.getLocVT().getSizeInBits() / 8;
            unsigned Offset = VA.getLocMemOffset();
            int FI = MFI->CreateFixedObject(Size, Offset, true);
            assert(Size == 4 && "Expected params to be converted to i32");

            // Store to this stack object
            SDValue FIN = DAG.getFrameIndex(FI, MVT::i32);
            MemOpChains.push_back(DAG.getStore(Chain, dl, Arg, FIN,
                MachinePointerInfo::getFixedStack(FI), false, false, 0));

            /*
             * Tail calls are only supported for functions with no stack arguments,
             * since the caller's stack frame is discarded upon entry to a tail
             * call. Look for non-regloc argument, and disable if we find any.
             */
            isTailCall = false;
        }
    }

    if (!MemOpChains.empty())
        Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other,
            &MemOpChains[0], MemOpChains.size());

    /*
     * Now copy input parameters to physical registers.
     *
     * These are all individually added to the Chain, after all
     * stores above have completed.
     */

    SDValue Glue;
    SmallVector<SDValue, 8> RegRefs;

    for (unsigned i = 0, end = ArgLocs.size(); i != end; i++) {
        CCValAssign &VA = ArgLocs[i];
        assert(VA.isMemLoc() || VA.isRegLoc());
        
        if (VA.isRegLoc()) {
            // Passed by register

            SDValue Arg = promoteArg(dl, DAG, VA, OutVals[i]);
            unsigned Reg = VA.getLocReg();
            Chain = DAG.getCopyToReg(Chain, dl, Reg, Arg, Glue);
            Glue = Chain.getValue(1);

            RegRefs.push_back(DAG.getRegister(Reg, Arg.getValueType()));
        }
    }

    /*
     * Emit the call DAG node
     */

    unsigned Opcode = resolveCallOpcode(dl, DAG, Callee, isTailCall);
    SmallVector<SDValue, 16> Ops;
    Ops.push_back(Chain);
    Ops.push_back(Callee);
    Ops.insert(Ops.end(), RegRefs.begin(), RegRefs.end());
    if (Glue.getNode())
        Ops.push_back(Glue);

    Chain = DAG.getNode(Opcode, dl, DAG.getVTList(MVT::Other, MVT::Glue),
        &Ops[0], Ops.size());

    // Result lowering isn't applicable for tail calls.
    if (isTailCall)
        return Chain;

    Glue = Chain.getValue(1);
    return LowerCallResult(Chain, Glue, CallConv, isVarArg, Ins, dl, DAG, InVals);
}

SDValue SVMTargetLowering::LowerCallResult(SDValue Chain, SDValue Glue,
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
        assert(VA.isRegLoc() && "Only register return values are supported");
        
        SDValue Val = DAG.getCopyFromReg(Chain, dl, VA.getLocReg(),
            VA.getLocVT(), Glue);
        Chain = Val.getValue(1);
        Glue = Val.getValue(2);

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
    SmallVector<CCValAssign, 16> RVLocs;
    CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
        getTargetMachine(), RVLocs, *DAG.getContext());
    CCInfo.AnalyzeReturn(Outs, RetCC_SVM);

    // Copy return values to physical output regs, and mark them as live

    SDValue Flag;
    for (unsigned i = 0; i != RVLocs.size(); ++i) {
        CCValAssign VA = RVLocs[i];
        assert(VA.isRegLoc() && "Only register return values are supported");

        Chain = DAG.getCopyToReg(Chain, dl, VA.getLocReg(), OutVals[i], Flag);
        Flag = Chain.getValue(1);
        
        DAG.getMachineFunction().getRegInfo().addLiveOut(VA.getLocReg());
    }

    if (Flag.getNode())
        return DAG.getNode(SVMISD::RETURN, dl, MVT::Other, Chain, Flag);
    else
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

MachineBasicBlock *SVMTargetLowering::EmitInstrWithCustomInserter(
    MachineInstr *MI, MachineBasicBlock *BB) const
{
    switch (MI->getOpcode()) {
    default:
        assert(false && "Unexpected instr type to insert");
        return NULL;
    case SVM::CMOV:
        return ExpandCMOV(MI, BB);
    }
    
}
