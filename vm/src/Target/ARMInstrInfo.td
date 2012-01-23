//===- ARMInstrInfo.td - Target Description for ARM Target -*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the ARM instructions in TableGen format.
//
//===----------------------------------------------------------------------===//

def SDT_ARMCmp     : SDTypeProfile<0, 2, [SDTCisSameAs<0, 1>]>;

def ARMWrapper       : SDNode<"ARMISD::Wrapper",     SDTIntUnaryOp>;

def ARMcmp           : SDNode<"ARMISD::CMP", SDT_ARMCmp,
                              [SDNPOutGlue]>;

def ARMcmpZ          : SDNode<"ARMISD::CMPZ", SDT_ARMCmp,
                              [SDNPOutGlue, SDNPCommutative]>;

//===----------------------------------------------------------------------===//
// ARM Flag Definitions.

class RegConstraint<string C> {
  string Constraints = C;
}

//===----------------------------------------------------------------------===//
//  ARM specific transformation functions and pattern fragments.
//

// so_imm_neg_XFORM - Return a so_imm value packed into the format described for
// so_imm_neg def below.
def so_imm_neg_XFORM : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(-(int)N->getZExtValue(), MVT::i32);
}]>;

// so_imm_not_XFORM - Return a so_imm value packed into the format described for
// so_imm_not def below.
def so_imm_not_XFORM : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(~(int)N->getZExtValue(), MVT::i32);
}]>;

/// imm1_15 predicate - True if the 32-bit immediate is in the range [1,15].
def imm1_15 : ImmLeaf<i32, [{
  return (int32_t)Imm >= 1 && (int32_t)Imm < 16;
}]>;

/// imm16_31 predicate - True if the 32-bit immediate is in the range [16,31].
def imm16_31 : ImmLeaf<i32, [{
  return (int32_t)Imm >= 16 && (int32_t)Imm < 32;
}]>;

def so_imm_neg :
  PatLeaf<(imm), [{
    return ARM_AM::getSOImmVal(-(uint32_t)N->getZExtValue()) != -1;
  }], so_imm_neg_XFORM>;

def so_imm_not :
  PatLeaf<(imm), [{
    return ARM_AM::getSOImmVal(~(uint32_t)N->getZExtValue()) != -1;
  }], so_imm_not_XFORM>;

// sext_16_node predicate - True if the SDNode is sign-extended 16 or more bits.
def sext_16_node : PatLeaf<(i32 GPR:$a), [{
  return CurDAG->ComputeNumSignBits(SDValue(N,0)) >= 17;
}]>;

/// Split a 32-bit immediate into two 16 bit parts.
def hi16 : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant((uint32_t)N->getZExtValue() >> 16, MVT::i32);
}]>;

def lo16AllZero : PatLeaf<(i32 imm), [{
  // Returns true if all low 16-bits are 0.
  return (((uint32_t)N->getZExtValue()) & 0xFFFFUL) == 0;
}], hi16>;

/// imm0_65535 - An immediate is in the range [0.65535].
def Imm0_65535AsmOperand: AsmOperandClass { let Name = "Imm0_65535"; }
def imm0_65535 : Operand<i32>, ImmLeaf<i32, [{
  return Imm >= 0 && Imm < 65536;
}]> {
  let ParserMatchClass = Imm0_65535AsmOperand;
}

class BinOpWithFlagFrag<dag res> :
      PatFrag<(ops node:$LHS, node:$RHS, node:$FLAG), res>;
class BinOpFrag<dag res> : PatFrag<(ops node:$LHS, node:$RHS), res>;
class UnOpFrag <dag res> : PatFrag<(ops node:$Src), res>;

// An 'and' node with a single use.
def and_su : PatFrag<(ops node:$lhs, node:$rhs), (and node:$lhs, node:$rhs), [{
  return N->hasOneUse();
}]>;

// An 'xor' node with a single use.
def xor_su : PatFrag<(ops node:$lhs, node:$rhs), (xor node:$lhs, node:$rhs), [{
  return N->hasOneUse();
}]>;

//===----------------------------------------------------------------------===//
// Operand Definitions.
//

// Branch target.
// FIXME: rename brtarget to t2_brtarget
def brtarget : Operand<OtherVT> {
  let EncoderMethod = "getBranchTargetOpValue";
  let OperandType = "OPERAND_PCREL";
  let DecoderMethod = "DecodeT2BROperand";
}

// FIXME: get rid of this one?
def uncondbrtarget : Operand<OtherVT> {
  let EncoderMethod = "getUnconditionalBranchTargetOpValue";
  let OperandType = "OPERAND_PCREL";
}

// Branch target for ARM. Handles conditional/unconditional
def br_target : Operand<OtherVT> {
  let EncoderMethod = "getARMBranchTargetOpValue";
  let OperandType = "OPERAND_PCREL";
}

// Call target.
// FIXME: rename bltarget to t2_bl_target?
def bltarget : Operand<i32> {
  // Encoded the same as branch targets.
  let EncoderMethod = "getBranchTargetOpValue";
  let OperandType = "OPERAND_PCREL";
}

// Call target for ARM. Handles conditional/unconditional
// FIXME: rename bl_target to t2_bltarget?
def bl_target : Operand<i32> {
  // Encoded the same as branch targets.
  let EncoderMethod = "getARMBranchTargetOpValue";
  let OperandType = "OPERAND_PCREL";
}

def blx_target : Operand<i32> {
  // Encoded the same as branch targets.
  let EncoderMethod = "getARMBLXTargetOpValue";
  let OperandType = "OPERAND_PCREL";
}

// A list of registers separated by comma. Used by load/store multiple.
def RegListAsmOperand : AsmOperandClass { let Name = "RegList"; }
def reglist : Operand<i32> {
  let EncoderMethod = "getRegisterListOpValue";
  let ParserMatchClass = RegListAsmOperand;
  let PrintMethod = "printRegisterList";
  let DecoderMethod = "DecodeRegListOperand";
}

def DPRRegListAsmOperand : AsmOperandClass { let Name = "DPRRegList"; }
def dpr_reglist : Operand<i32> {
  let EncoderMethod = "getRegisterListOpValue";
  let ParserMatchClass = DPRRegListAsmOperand;
  let PrintMethod = "printRegisterList";
  let DecoderMethod = "DecodeDPRRegListOperand";
}

def SPRRegListAsmOperand : AsmOperandClass { let Name = "SPRRegList"; }
def spr_reglist : Operand<i32> {
  let EncoderMethod = "getRegisterListOpValue";
  let ParserMatchClass = SPRRegListAsmOperand;
  let PrintMethod = "printRegisterList";
  let DecoderMethod = "DecodeSPRRegListOperand";
}

// An operand for the CONSTPOOL_ENTRY pseudo-instruction.
def cpinst_operand : Operand<i32> {
  let PrintMethod = "printCPInstOperand";
}

// Local PC labels.
def pclabel : Operand<i32> {
  let PrintMethod = "printPCLabel";
}

// ADR instruction labels.
def adrlabel : Operand<i32> {
  let EncoderMethod = "getAdrLabelOpValue";
}

def neon_vcvt_imm32 : Operand<i32> {
  let EncoderMethod = "getNEONVcvtImm32OpValue";
  let DecoderMethod = "DecodeVCVTImmOperand";
}

// rot_imm: An integer that encodes a rotate amount. Must be 8, 16, or 24.
def rot_imm_XFORM: SDNodeXForm<imm, [{
  switch (N->getZExtValue()){
  default: assert(0);
  case 0:  return CurDAG->getTargetConstant(0, MVT::i32);
  case 8:  return CurDAG->getTargetConstant(1, MVT::i32);
  case 16: return CurDAG->getTargetConstant(2, MVT::i32);
  case 24: return CurDAG->getTargetConstant(3, MVT::i32);
  }
}]>;
def RotImmAsmOperand : AsmOperandClass {
  let Name = "RotImm";
  let ParserMethod = "parseRotImm";
}
def rot_imm : Operand<i32>, PatLeaf<(i32 imm), [{
    int32_t v = N->getZExtValue();
    return v == 8 || v == 16 || v == 24; }],
    rot_imm_XFORM> {
  let PrintMethod = "printRotImmOperand";
  let ParserMatchClass = RotImmAsmOperand;
}

// shift_imm: An integer that encodes a shift amount and the type of shift
// (asr or lsl). The 6-bit immediate encodes as:
//    {5}     0 ==> lsl
//            1     asr
//    {4-0}   imm5 shift amount.
//            asr #32 encoded as imm5 == 0.
def ShifterImmAsmOperand : AsmOperandClass {
  let Name = "ShifterImm";
  let ParserMethod = "parseShifterImm";
}
def shift_imm : Operand<i32> {
  let PrintMethod = "printShiftImmOperand";
  let ParserMatchClass = ShifterImmAsmOperand;
}

// shifter_operand operands: so_reg_reg, so_reg_imm, and so_imm.
def ShiftedRegAsmOperand : AsmOperandClass { let Name = "RegShiftedReg"; }
def so_reg_reg : Operand<i32>,  // reg reg imm
                 ComplexPattern<i32, 3, "SelectRegShifterOperand",
                                [shl, srl, sra, rotr]> {
  let EncoderMethod = "getSORegRegOpValue";
  let PrintMethod = "printSORegRegOperand";
  let DecoderMethod = "DecodeSORegRegOperand";
  let ParserMatchClass = ShiftedRegAsmOperand;
  let MIOperandInfo = (ops GPRnopc, GPRnopc, i32imm);
}

def ShiftedImmAsmOperand : AsmOperandClass { let Name = "RegShiftedImm"; }
def so_reg_imm : Operand<i32>, // reg imm
                 ComplexPattern<i32, 2, "SelectImmShifterOperand",
                                [shl, srl, sra, rotr]> {
  let EncoderMethod = "getSORegImmOpValue";
  let PrintMethod = "printSORegImmOperand";
  let DecoderMethod = "DecodeSORegImmOperand";
  let ParserMatchClass = ShiftedImmAsmOperand;
  let MIOperandInfo = (ops GPR, i32imm);
}

// FIXME: Does this need to be distinct from so_reg?
def shift_so_reg_reg : Operand<i32>,    // reg reg imm
                   ComplexPattern<i32, 3, "SelectShiftRegShifterOperand",
                                  [shl,srl,sra,rotr]> {
  let EncoderMethod = "getSORegRegOpValue";
  let PrintMethod = "printSORegRegOperand";
  let DecoderMethod = "DecodeSORegRegOperand";
  let MIOperandInfo = (ops GPR, GPR, i32imm);
}

// FIXME: Does this need to be distinct from so_reg?
def shift_so_reg_imm : Operand<i32>,    // reg reg imm
                   ComplexPattern<i32, 2, "SelectShiftImmShifterOperand",
                                  [shl,srl,sra,rotr]> {
  let EncoderMethod = "getSORegImmOpValue";
  let PrintMethod = "printSORegImmOperand";
  let DecoderMethod = "DecodeSORegImmOperand";
  let MIOperandInfo = (ops GPR, i32imm);
}


// so_imm - Match a 32-bit shifter_operand immediate operand, which is an
// 8-bit immediate rotated by an arbitrary number of bits.
def SOImmAsmOperand: AsmOperandClass { let Name = "ARMSOImm"; }
def so_imm : Operand<i32>, ImmLeaf<i32, [{
    return ARM_AM::getSOImmVal(Imm) != -1;
  }]> {
  let EncoderMethod = "getSOImmOpValue";
  let ParserMatchClass = SOImmAsmOperand;
  let DecoderMethod = "DecodeSOImmOperand";
}

// Break so_imm's up into two pieces.  This handles immediates with up to 16
// bits set in them.  This uses so_imm2part to match and so_imm2part_[12] to
// get the first/second pieces.
def so_imm2part : PatLeaf<(imm), [{
      return ARM_AM::isSOImmTwoPartVal((unsigned)N->getZExtValue());
}]>;

/// arm_i32imm - True for +V6T2, or true only if so_imm2part is true.
///
def arm_i32imm : PatLeaf<(imm), [{
  if (Subtarget->hasV6T2Ops())
    return true;
  return ARM_AM::isSOImmTwoPartVal((unsigned)N->getZExtValue());
}]>;

/// imm0_7 predicate - Immediate in the range [0,7].
def Imm0_7AsmOperand: AsmOperandClass { let Name = "Imm0_7"; }
def imm0_7 : Operand<i32>, ImmLeaf<i32, [{
  return Imm >= 0 && Imm < 8;
}]> {
  let ParserMatchClass = Imm0_7AsmOperand;
}

/// imm0_15 predicate - Immediate in the range [0,15].
def Imm0_15AsmOperand: AsmOperandClass { let Name = "Imm0_15"; }
def imm0_15 : Operand<i32>, ImmLeaf<i32, [{
  return Imm >= 0 && Imm < 16;
}]> {
  let ParserMatchClass = Imm0_15AsmOperand;
}

/// imm0_31 predicate - True if the 32-bit immediate is in the range [0,31].
def Imm0_31AsmOperand: AsmOperandClass { let Name = "Imm0_31"; }
def imm0_31 : Operand<i32>, ImmLeaf<i32, [{
  return Imm >= 0 && Imm < 32;
}]> {
  let ParserMatchClass = Imm0_31AsmOperand;
}

/// imm0_255 predicate - Immediate in the range [0,255].
def Imm0_255AsmOperand : AsmOperandClass { let Name = "Imm0_255"; }
def imm0_255 : Operand<i32>, ImmLeaf<i32, [{ return Imm >= 0 && Imm < 256; }]> {
  let ParserMatchClass = Imm0_255AsmOperand;
}

// imm0_65535_expr - For movt/movw - 16-bit immediate that can also reference
// a relocatable expression.
//
// FIXME: This really needs a Thumb version separate from the ARM version.
// While the range is the same, and can thus use the same match class,
// the encoding is different so it should have a different encoder method.
def Imm0_65535ExprAsmOperand: AsmOperandClass { let Name = "Imm0_65535Expr"; }
def imm0_65535_expr : Operand<i32> {
  let EncoderMethod = "getHiLo16ImmOpValue";
  let ParserMatchClass = Imm0_65535ExprAsmOperand;
}

/// imm24b - True if the 32-bit immediate is encodable in 24 bits.
def Imm24bitAsmOperand: AsmOperandClass { let Name = "Imm24bit"; }
def imm24b : Operand<i32>, ImmLeaf<i32, [{
  return Imm >= 0 && Imm <= 0xffffff;
}]> {
  let ParserMatchClass = Imm24bitAsmOperand;
}


/// bf_inv_mask_imm predicate - An AND mask to clear an arbitrary width bitfield
/// e.g., 0xf000ffff
def BitfieldAsmOperand : AsmOperandClass {
  let Name = "Bitfield";
  let ParserMethod = "parseBitfield";
}
def bf_inv_mask_imm : Operand<i32>,
                      PatLeaf<(imm), [{
  return ARM::isBitFieldInvertedMask(N->getZExtValue());
}] > {
  let EncoderMethod = "getBitfieldInvertedMaskOpValue";
  let PrintMethod = "printBitfieldInvMaskImmOperand";
  let DecoderMethod = "DecodeBitfieldMaskOperand";
  let ParserMatchClass = BitfieldAsmOperand;
}

def imm1_32_XFORM: SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant((int)N->getZExtValue() - 1, MVT::i32);
}]>;
def Imm1_32AsmOperand: AsmOperandClass { let Name = "Imm1_32"; }
def imm1_32 : Operand<i32>, PatLeaf<(imm), [{
   uint64_t Imm = N->getZExtValue();
   return Imm > 0 && Imm <= 32;
 }],
    imm1_32_XFORM> {
  let PrintMethod = "printImmPlusOneOperand";
  let ParserMatchClass = Imm1_32AsmOperand;
}

def imm1_16_XFORM: SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant((int)N->getZExtValue() - 1, MVT::i32);
}]>;
def Imm1_16AsmOperand: AsmOperandClass { let Name = "Imm1_16"; }
def imm1_16 : Operand<i32>, PatLeaf<(imm), [{ return Imm > 0 && Imm <= 16; }],
    imm1_16_XFORM> {
  let PrintMethod = "printImmPlusOneOperand";
  let ParserMatchClass = Imm1_16AsmOperand;
}

// Define ARM specific addressing modes.
// addrmode_imm12 := reg +/- imm12
//
def MemImm12OffsetAsmOperand : AsmOperandClass { let Name = "MemImm12Offset"; }
def addrmode_imm12 : Operand<i32>,
                     ComplexPattern<i32, 2, "SelectAddrModeImm12", []> {
  // 12-bit immediate operand. Note that instructions using this encode
  // #0 and #-0 differently. We flag #-0 as the magic value INT32_MIN. All other
  // immediate values are as normal.

  let EncoderMethod = "getAddrModeImm12OpValue";
  let PrintMethod = "printAddrModeImm12Operand";
  let DecoderMethod = "DecodeAddrModeImm12Operand";
  let ParserMatchClass = MemImm12OffsetAsmOperand;
  let MIOperandInfo = (ops GPR:$base, i32imm:$offsimm);
}
// ldst_so_reg := reg +/- reg shop imm
//
def MemRegOffsetAsmOperand : AsmOperandClass { let Name = "MemRegOffset"; }
def ldst_so_reg : Operand<i32>,
                  ComplexPattern<i32, 3, "SelectLdStSOReg", []> {
  let EncoderMethod = "getLdStSORegOpValue";
  // FIXME: Simplify the printer
  let PrintMethod = "printAddrMode2Operand";
  let DecoderMethod = "DecodeSORegMemOperand";
  let ParserMatchClass = MemRegOffsetAsmOperand;
  let MIOperandInfo = (ops GPR:$base, GPRnopc:$offsreg, i32imm:$shift);
}

// postidx_imm8 := +/- [0,255]
//
// 9 bit value:
//  {8}       1 is imm8 is non-negative. 0 otherwise.
//  {7-0}     [0,255] imm8 value.
def PostIdxImm8AsmOperand : AsmOperandClass { let Name = "PostIdxImm8"; }
def postidx_imm8 : Operand<i32> {
  let PrintMethod = "printPostIdxImm8Operand";
  let ParserMatchClass = PostIdxImm8AsmOperand;
  let MIOperandInfo = (ops i32imm);
}

// postidx_imm8s4 := +/- [0,1020]
//
// 9 bit value:
//  {8}       1 is imm8 is non-negative. 0 otherwise.
//  {7-0}     [0,255] imm8 value, scaled by 4.
def PostIdxImm8s4AsmOperand : AsmOperandClass { let Name = "PostIdxImm8s4"; }
def postidx_imm8s4 : Operand<i32> {
  let PrintMethod = "printPostIdxImm8s4Operand";
  let ParserMatchClass = PostIdxImm8s4AsmOperand;
  let MIOperandInfo = (ops i32imm);
}


// postidx_reg := +/- reg
//
def PostIdxRegAsmOperand : AsmOperandClass {
  let Name = "PostIdxReg";
  let ParserMethod = "parsePostIdxReg";
}
def postidx_reg : Operand<i32> {
  let EncoderMethod = "getPostIdxRegOpValue";
  let DecoderMethod = "DecodePostIdxReg";
  let PrintMethod = "printPostIdxRegOperand";
  let ParserMatchClass = PostIdxRegAsmOperand;
  let MIOperandInfo = (ops GPR, i32imm);
}


// addrmode2 := reg +/- imm12
//           := reg +/- reg shop imm
//
// FIXME: addrmode2 should be refactored the rest of the way to always
// use explicit imm vs. reg versions above (addrmode_imm12 and ldst_so_reg).
def AddrMode2AsmOperand : AsmOperandClass { let Name = "AddrMode2"; }
def addrmode2 : Operand<i32>,
                ComplexPattern<i32, 3, "SelectAddrMode2", []> {
  let EncoderMethod = "getAddrMode2OpValue";
  let PrintMethod = "printAddrMode2Operand";
  let ParserMatchClass = AddrMode2AsmOperand;
  let MIOperandInfo = (ops GPR:$base, GPR:$offsreg, i32imm:$offsimm);
}

def PostIdxRegShiftedAsmOperand : AsmOperandClass {
  let Name = "PostIdxRegShifted";
  let ParserMethod = "parsePostIdxReg";
}
def am2offset_reg : Operand<i32>,
                ComplexPattern<i32, 2, "SelectAddrMode2OffsetReg",
                [], [SDNPWantRoot]> {
  let EncoderMethod = "getAddrMode2OffsetOpValue";
  let PrintMethod = "printAddrMode2OffsetOperand";
  // When using this for assembly, it's always as a post-index offset.
  let ParserMatchClass = PostIdxRegShiftedAsmOperand;
  let MIOperandInfo = (ops GPR, i32imm);
}

// FIXME: am2offset_imm should only need the immediate, not the GPR. Having
// the GPR is purely vestigal at this point.
def AM2OffsetImmAsmOperand : AsmOperandClass { let Name = "AM2OffsetImm"; }
def am2offset_imm : Operand<i32>,
                ComplexPattern<i32, 2, "SelectAddrMode2OffsetImm",
                [], [SDNPWantRoot]> {
  let EncoderMethod = "getAddrMode2OffsetOpValue";
  let PrintMethod = "printAddrMode2OffsetOperand";
  let ParserMatchClass = AM2OffsetImmAsmOperand;
  let MIOperandInfo = (ops GPR, i32imm);
}


// addrmode3 := reg +/- reg
// addrmode3 := reg +/- imm8
//
// FIXME: split into imm vs. reg versions.
def AddrMode3AsmOperand : AsmOperandClass { let Name = "AddrMode3"; }
def addrmode3 : Operand<i32>,
                ComplexPattern<i32, 3, "SelectAddrMode3", []> {
  let EncoderMethod = "getAddrMode3OpValue";
  let PrintMethod = "printAddrMode3Operand";
  let ParserMatchClass = AddrMode3AsmOperand;
  let MIOperandInfo = (ops GPR:$base, GPR:$offsreg, i32imm:$offsimm);
}

// FIXME: split into imm vs. reg versions.
// FIXME: parser method to handle +/- register.
def AM3OffsetAsmOperand : AsmOperandClass {
  let Name = "AM3Offset";
  let ParserMethod = "parseAM3Offset";
}
def am3offset : Operand<i32>,
                ComplexPattern<i32, 2, "SelectAddrMode3Offset",
                               [], [SDNPWantRoot]> {
  let EncoderMethod = "getAddrMode3OffsetOpValue";
  let PrintMethod = "printAddrMode3OffsetOperand";
  let ParserMatchClass = AM3OffsetAsmOperand;
  let MIOperandInfo = (ops GPR, i32imm);
}

// ldstm_mode := {ia, ib, da, db}
//
def ldstm_mode : OptionalDefOperand<OtherVT, (ops i32), (ops (i32 1))> {
  let EncoderMethod = "getLdStmModeOpValue";
  let PrintMethod = "printLdStmModeOperand";
}

// addrmode5 := reg +/- imm8*4
//
def AddrMode5AsmOperand : AsmOperandClass { let Name = "AddrMode5"; }
def addrmode5 : Operand<i32>,
                ComplexPattern<i32, 2, "SelectAddrMode5", []> {
  let PrintMethod = "printAddrMode5Operand";
  let EncoderMethod = "getAddrMode5OpValue";
  let DecoderMethod = "DecodeAddrMode5Operand";
  let ParserMatchClass = AddrMode5AsmOperand;
  let MIOperandInfo = (ops GPR:$base, i32imm);
}

// addrmode6 := reg with optional alignment
//
def AddrMode6AsmOperand : AsmOperandClass { let Name = "AlignedMemory"; }
def addrmode6 : Operand<i32>,
                ComplexPattern<i32, 2, "SelectAddrMode6", [], [SDNPWantParent]>{
  let PrintMethod = "printAddrMode6Operand";
  let MIOperandInfo = (ops GPR:$addr, i32imm:$align);
  let EncoderMethod = "getAddrMode6AddressOpValue";
  let DecoderMethod = "DecodeAddrMode6Operand";
  let ParserMatchClass = AddrMode6AsmOperand;
}

def am6offset : Operand<i32>,
                ComplexPattern<i32, 1, "SelectAddrMode6Offset",
                               [], [SDNPWantRoot]> {
  let PrintMethod = "printAddrMode6OffsetOperand";
  let MIOperandInfo = (ops GPR);
  let EncoderMethod = "getAddrMode6OffsetOpValue";
  let DecoderMethod = "DecodeGPRRegisterClass";
}

// Special version of addrmode6 to handle alignment encoding for VST1/VLD1
// (single element from one lane) for size 32.
def addrmode6oneL32 : Operand<i32>,
                ComplexPattern<i32, 2, "SelectAddrMode6", [], [SDNPWantParent]>{
  let PrintMethod = "printAddrMode6Operand";
  let MIOperandInfo = (ops GPR:$addr, i32imm);
  let EncoderMethod = "getAddrMode6OneLane32AddressOpValue";
}

// Special version of addrmode6 to handle alignment encoding for VLD-dup
// instructions, specifically VLD4-dup.
def addrmode6dup : Operand<i32>,
                ComplexPattern<i32, 2, "SelectAddrMode6", [], [SDNPWantParent]>{
  let PrintMethod = "printAddrMode6Operand";
  let MIOperandInfo = (ops GPR:$addr, i32imm);
  let EncoderMethod = "getAddrMode6DupAddressOpValue";
}

// addrmodepc := pc + reg
//
def addrmodepc : Operand<i32>,
                 ComplexPattern<i32, 2, "SelectAddrModePC", []> {
  let PrintMethod = "printAddrModePCOperand";
  let MIOperandInfo = (ops GPR, i32imm);
}

// addr_offset_none := reg
//
def MemNoOffsetAsmOperand : AsmOperandClass { let Name = "MemNoOffset"; }
def addr_offset_none : Operand<i32>,
                       ComplexPattern<i32, 1, "SelectAddrOffsetNone", []> {
  let PrintMethod = "printAddrMode7Operand";
  let DecoderMethod = "DecodeAddrMode7Operand";
  let ParserMatchClass = MemNoOffsetAsmOperand;
  let MIOperandInfo = (ops GPR:$base);
}

def nohash_imm : Operand<i32> {
  let PrintMethod = "printNoHashImmediate";
}


//===----------------------------------------------------------------------===//

include "ARMInstrFormats.td"


//===----------------------------------------------------------------------===//
// Thumb Support
//

include "ARMInstrThumb.td"
