//===- ARMInstrThumb.td - Thumb support for ARM ------------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the Thumb instruction set.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Thumb specific DAG Nodes.
//

def imm_sr_XFORM: SDNodeXForm<imm, [{
  unsigned Imm = N->getZExtValue();
  return CurDAG->getTargetConstant((Imm == 32 ? 0 : Imm), MVT::i32);
}]>;
def ThumbSRImmAsmOperand: AsmOperandClass { let Name = "ImmThumbSR"; }
def imm_sr : Operand<i32>, PatLeaf<(imm), [{
  uint64_t Imm = N->getZExtValue();
  return Imm > 0 && Imm <= 32;
}], imm_sr_XFORM> {
  let PrintMethod = "printThumbSRImm";
  let ParserMatchClass = ThumbSRImmAsmOperand;
}

def imm_neg_XFORM : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(-(int)N->getZExtValue(), MVT::i32);
}]>;
def imm_comp_XFORM : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(~((uint32_t)N->getZExtValue()), MVT::i32);
}]>;

def imm0_7_neg : PatLeaf<(i32 imm), [{
  return (uint32_t)-N->getZExtValue() < 8;
}], imm_neg_XFORM>;

def imm0_255_comp : PatLeaf<(i32 imm), [{
  return ~((uint32_t)N->getZExtValue()) < 256;
}]>;

def imm8_255 : ImmLeaf<i32, [{
  return Imm >= 8 && Imm < 256;
}]>;
def imm8_255_neg : PatLeaf<(i32 imm), [{
  unsigned Val = -N->getZExtValue();
  return Val >= 8 && Val < 256;
}], imm_neg_XFORM>;

// Break imm's up into two pieces: an immediate + a left shift. This uses
// thumb_immshifted to match and thumb_immshifted_val and thumb_immshifted_shamt
// to get the val/shift pieces.
def thumb_immshifted : PatLeaf<(imm), [{
  return ARM_AM::isThumbImmShiftedVal((unsigned)N->getZExtValue());
}]>;

def thumb_immshifted_val : SDNodeXForm<imm, [{
  unsigned V = ARM_AM::getThumbImmNonShiftedVal((unsigned)N->getZExtValue());
  return CurDAG->getTargetConstant(V, MVT::i32);
}]>;

def thumb_immshifted_shamt : SDNodeXForm<imm, [{
  unsigned V = ARM_AM::getThumbImmValShift((unsigned)N->getZExtValue());
  return CurDAG->getTargetConstant(V, MVT::i32);
}]>;

// ADR instruction labels.
def t_adrlabel : Operand<i32> {
  let EncoderMethod = "getThumbAdrLabelOpValue";
}

// Scaled 4 immediate.
def t_imm0_1020s4_asmoperand: AsmOperandClass { let Name = "Imm0_1020s4"; }
def t_imm0_1020s4 : Operand<i32> {
  let PrintMethod = "printThumbS4ImmOperand";
  let ParserMatchClass = t_imm0_1020s4_asmoperand;
  let OperandType = "OPERAND_IMMEDIATE";
}

def t_imm0_508s4_asmoperand: AsmOperandClass { let Name = "Imm0_508s4"; }
def t_imm0_508s4 : Operand<i32> {
  let PrintMethod = "printThumbS4ImmOperand";
  let ParserMatchClass = t_imm0_508s4_asmoperand;
  let OperandType = "OPERAND_IMMEDIATE";
}

// Define Thumb specific addressing modes.

let OperandType = "OPERAND_PCREL" in {
def t_brtarget : Operand<OtherVT> {
  let EncoderMethod = "getThumbBRTargetOpValue";
  let DecoderMethod = "DecodeThumbBROperand";
}

def t_bcctarget : Operand<i32> {
  let EncoderMethod = "getThumbBCCTargetOpValue";
  let DecoderMethod = "DecodeThumbBCCTargetOperand";
}

def t_cbtarget : Operand<i32> {
  let EncoderMethod = "getThumbCBTargetOpValue";
  let DecoderMethod = "DecodeThumbCmpBROperand";
}

def t_bltarget : Operand<i32> {
  let EncoderMethod = "getThumbBLTargetOpValue";
  let DecoderMethod = "DecodeThumbBLTargetOperand";
}

def t_blxtarget : Operand<i32> {
  let EncoderMethod = "getThumbBLXTargetOpValue";
  let DecoderMethod = "DecodeThumbBLXOffset";
}
}

// t_addrmode_rr := reg + reg
//
def t_addrmode_rr_asm_operand : AsmOperandClass { let Name = "MemThumbRR"; }
def t_addrmode_rr : Operand<i32>,
                    ComplexPattern<i32, 2, "SelectThumbAddrModeRR", []> {
  let EncoderMethod = "getThumbAddrModeRegRegOpValue";
  let PrintMethod = "printThumbAddrModeRROperand";
  let DecoderMethod = "DecodeThumbAddrModeRR";
  let ParserMatchClass = t_addrmode_rr_asm_operand;
  let MIOperandInfo = (ops tGPR:$base, tGPR:$offsreg);
}

// t_addrmode_rrs := reg + reg
//
// We use separate scaled versions because the Select* functions need
// to explicitly check for a matching constant and return false here so that
// the reg+imm forms will match instead. This is a horrible way to do that,
// as it forces tight coupling between the methods, but it's how selectiondag
// currently works.
def t_addrmode_rrs1 : Operand<i32>,
                      ComplexPattern<i32, 2, "SelectThumbAddrModeRI5S1", []> {
  let EncoderMethod = "getThumbAddrModeRegRegOpValue";
  let PrintMethod = "printThumbAddrModeRROperand";
  let DecoderMethod = "DecodeThumbAddrModeRR";
  let ParserMatchClass = t_addrmode_rr_asm_operand;
  let MIOperandInfo = (ops tGPR:$base, tGPR:$offsreg);
}
def t_addrmode_rrs2 : Operand<i32>,
                      ComplexPattern<i32, 2, "SelectThumbAddrModeRI5S2", []> {
  let EncoderMethod = "getThumbAddrModeRegRegOpValue";
  let DecoderMethod = "DecodeThumbAddrModeRR";
  let PrintMethod = "printThumbAddrModeRROperand";
  let ParserMatchClass = t_addrmode_rr_asm_operand;
  let MIOperandInfo = (ops tGPR:$base, tGPR:$offsreg);
}
def t_addrmode_rrs4 : Operand<i32>,
                      ComplexPattern<i32, 2, "SelectThumbAddrModeRI5S4", []> {
  let EncoderMethod = "getThumbAddrModeRegRegOpValue";
  let DecoderMethod = "DecodeThumbAddrModeRR";
  let PrintMethod = "printThumbAddrModeRROperand";
  let ParserMatchClass = t_addrmode_rr_asm_operand;
  let MIOperandInfo = (ops tGPR:$base, tGPR:$offsreg);
}

// t_addrmode_is4 := reg + imm5 * 4
//
def t_addrmode_is4_asm_operand : AsmOperandClass { let Name = "MemThumbRIs4"; }
def t_addrmode_is4 : Operand<i32>,
                     ComplexPattern<i32, 2, "SelectThumbAddrModeImm5S4", []> {
  let EncoderMethod = "getAddrModeISOpValue";
  let DecoderMethod = "DecodeThumbAddrModeIS";
  let PrintMethod = "printThumbAddrModeImm5S4Operand";
  let ParserMatchClass = t_addrmode_is4_asm_operand;
  let MIOperandInfo = (ops tGPR:$base, i32imm:$offsimm);
}

// t_addrmode_is2 := reg + imm5 * 2
//
def t_addrmode_is2_asm_operand : AsmOperandClass { let Name = "MemThumbRIs2"; }
def t_addrmode_is2 : Operand<i32>,
                     ComplexPattern<i32, 2, "SelectThumbAddrModeImm5S2", []> {
  let EncoderMethod = "getAddrModeISOpValue";
  let DecoderMethod = "DecodeThumbAddrModeIS";
  let PrintMethod = "printThumbAddrModeImm5S2Operand";
  let ParserMatchClass = t_addrmode_is2_asm_operand;
  let MIOperandInfo = (ops tGPR:$base, i32imm:$offsimm);
}

// t_addrmode_is1 := reg + imm5
//
def t_addrmode_is1_asm_operand : AsmOperandClass { let Name = "MemThumbRIs1"; }
def t_addrmode_is1 : Operand<i32>,
                     ComplexPattern<i32, 2, "SelectThumbAddrModeImm5S1", []> {
  let EncoderMethod = "getAddrModeISOpValue";
  let DecoderMethod = "DecodeThumbAddrModeIS";
  let PrintMethod = "printThumbAddrModeImm5S1Operand";
  let ParserMatchClass = t_addrmode_is1_asm_operand;
  let MIOperandInfo = (ops tGPR:$base, i32imm:$offsimm);
}

// t_addrmode_sp := sp + imm8 * 4
//
// FIXME: This really shouldn't have an explicit SP operand at all. It should
// be implicit, just like in the instruction encoding itself.
def t_addrmode_sp_asm_operand : AsmOperandClass { let Name = "MemThumbSPI"; }
def t_addrmode_sp : Operand<i32>,
                    ComplexPattern<i32, 2, "SelectThumbAddrModeSP", []> {
  let EncoderMethod = "getAddrModeThumbSPOpValue";
  let DecoderMethod = "DecodeThumbAddrModeSP";
  let PrintMethod = "printThumbAddrModeSPOperand";
  let ParserMatchClass = t_addrmode_sp_asm_operand;
  let MIOperandInfo = (ops GPR:$base, i32imm:$offsimm);
}

// t_addrmode_pc := <label> => pc + imm8 * 4
//
def t_addrmode_pc : Operand<i32> {
  let EncoderMethod = "getAddrModePCOpValue";
  let DecoderMethod = "DecodeThumbAddrModePC";
}

//===----------------------------------------------------------------------===//
//  Miscellaneous Instructions.
//

class T1SystemEncoding<bits<8> opc>
  : T1Encoding<0b101111> {
  let Inst{9-8} = 0b11;
  let Inst{7-0} = opc;
}

def tYIELD : T1pI<(outs), (ins), NoItinerary, "yield", "", []>,
           T1SystemEncoding<0x10>; // A8.6.410

def tWFE : T1pI<(outs), (ins), NoItinerary, "wfe", "", []>,
           T1SystemEncoding<0x20>; // A8.6.408

def tWFI : T1pI<(outs), (ins), NoItinerary, "wfi", "", []>,
           T1SystemEncoding<0x30>; // A8.6.409

def tSEV : T1pI<(outs), (ins), NoItinerary, "sev", "", []>,
           T1SystemEncoding<0x40>; // A8.6.157

// The imm operand $val can be used by a debugger to store more information
// about the breakpoint.
def tBKPT : T1I<(outs), (ins imm0_255:$val), NoItinerary, "bkpt\t$val",
                []>,
           T1Encoding<0b101111> {
  let Inst{9-8} = 0b10;
  // A8.6.22
  bits<8> val;
  let Inst{7-0} = val;
}

// ADD <Rd>, sp, #<imm8>
// FIXME: This should not be marked as having side effects, and it should be
// rematerializable. Clearing the side effect bit causes miscompilations,
// probably because the instruction can be moved around.
def tADDrSPi : T1pI<(outs tGPR:$dst), (ins GPRsp:$sp, t_imm0_1020s4:$imm),
                    IIC_iALUi, "add", "\t$dst, $sp, $imm", []>,
               T1Encoding<{1,0,1,0,1,?}> {
  // A6.2 & A8.6.8
  bits<3> dst;
  bits<8> imm;
  let Inst{10-8} = dst;
  let Inst{7-0}  = imm;
  let DecoderMethod = "DecodeThumbAddSpecialReg";
}

let neverHasSideEffects = 1, isReMaterializable = 1 in
def tLEApcrel   : tPseudoInst<(outs tGPR:$Rd), (ins i32imm:$label, pred:$p),
                              2, IIC_iALUi, []>;

def tLEApcrelJT : tPseudoInst<(outs tGPR:$Rd),
                              (ins i32imm:$label, nohash_imm:$id, pred:$p),
                              2, IIC_iALUi, []>;


// ADD sp, sp, #<imm7>
def tADDspi : T1pIt<(outs GPRsp:$Rdn), (ins GPRsp:$Rn, t_imm0_508s4:$imm),
                     IIC_iALUi, "add", "\t$Rdn, $imm", []>,
              T1Misc<{0,0,0,0,0,?,?}> {
  // A6.2.5 & A8.6.8
  bits<7> imm;
  let Inst{6-0} = imm;
  let DecoderMethod = "DecodeThumbAddSPImm";
}

// SUB sp, sp, #<imm7>
// FIXME: The encoding and the ASM string don't match up.
def tSUBspi : T1pIt<(outs GPRsp:$Rdn), (ins GPRsp:$Rn, t_imm0_508s4:$imm),
                    IIC_iALUi, "sub", "\t$Rdn, $imm", []>,
              T1Misc<{0,0,0,0,1,?,?}> {
  // A6.2.5 & A8.6.214
  bits<7> imm;
  let Inst{6-0} = imm;
  let DecoderMethod = "DecodeThumbAddSPImm";
}

// Can optionally specify SP as a three operand instruction.
def : InstAlias<"add${p} sp, sp, $imm",
                 (tADDspi SP, t_imm0_508s4:$imm, pred:$p)>;
def : InstAlias<"sub${p} sp, sp, $imm",
                 (tSUBspi SP, t_imm0_508s4:$imm, pred:$p)>;

// ADD <Rm>, sp
def tADDrSP : T1pIt<(outs GPR:$Rdn), (ins GPR:$Rn, GPRsp:$sp), IIC_iALUr,
                  "add", "\t$Rdn, $sp, $Rn", []>,
              T1Special<{0,0,?,?}> {
  // A8.6.9 Encoding T1
  bits<4> Rdn;
  let Inst{7}   = Rdn{3};
  let Inst{6-3} = 0b1101;
  let Inst{2-0} = Rdn{2-0};
  let DecoderMethod = "DecodeThumbAddSPReg";
}

// ADD sp, <Rm>
def tADDspr : T1pIt<(outs GPRsp:$Rdn), (ins GPRsp:$Rn, GPR:$Rm), IIC_iALUr,
                  "add", "\t$Rdn, $Rm", []>,
              T1Special<{0,0,?,?}> {
  // A8.6.9 Encoding T2
  bits<4> Rm;
  let Inst{7} = 1;
  let Inst{6-3} = Rm;
  let Inst{2-0} = 0b101;
  let DecoderMethod = "DecodeThumbAddSPReg";
}

//===----------------------------------------------------------------------===//
//  Control Flow Instructions.
//

// Indirect branches
let isBranch = 1, isTerminator = 1, isBarrier = 1, isIndirectBranch = 1 in {
  def tBX : TI<(outs), (ins GPR:$Rm, pred:$p), IIC_Br, "bx${p}\t$Rm", []>,
            T1Special<{1,1,0,?}> {
    // A6.2.3 & A8.6.25
    bits<4> Rm;
    let Inst{6-3} = Rm;
    let Inst{2-0} = 0b000;
  }
}

let isBranch = 1, isTerminator = 1, isBarrier = 1 in {
  let isPredicable = 1 in
  def tB   : T1pI<(outs), (ins t_brtarget:$target), IIC_Br,
                 "b", "\t$target", [(br bb:$target)]>,
             T1Encoding<{1,1,1,0,0,?}> {
    bits<11> target;
    let Inst{10-0} = target;
  }
}

// FIXME: should be able to write a pattern for ARMBrcond, but can't use
// a two-value operand where a dag node expects two operands. :(
let isBranch = 1, isTerminator = 1 in
  def tBcc : T1I<(outs), (ins t_bcctarget:$target, pred:$p), IIC_Br,
                 "b${p}\t$target",
                 [/*(ARMbrcond bb:$target, imm:$cc)*/]>,
             T1BranchCond<{1,1,0,1}> {
  bits<4> p;
  bits<8> target;
  let Inst{11-8} = p;
  let Inst{7-0} = target;
}

// A8.6.218 Supervisor Call (Software Interrupt)
// A8.6.16 B: Encoding T1
// If Inst{11-8} == 0b1111 then SEE SVC
let isCall = 1, Uses = [SP] in
def tSVC : T1pI<(outs), (ins imm0_255:$imm), IIC_Br,
                "svc", "\t$imm", []>, Encoding16 {
  bits<8> imm;
  let Inst{15-12} = 0b1101;
  let Inst{11-8}  = 0b1111;
  let Inst{7-0}   = imm;
}

// The assembler uses 0xDEFE for a trap instruction.
let isBarrier = 1, isTerminator = 1 in
def tTRAP : TI<(outs), (ins), IIC_Br,
               "trap", [(trap)]>, Encoding16 {
  let Inst = 0xdefe;
}

//===----------------------------------------------------------------------===//
//  Load Store Instructions.
//

// Loads: reg/reg and reg/imm5
let canFoldAsLoad = 1, isReMaterializable = 1 in
multiclass thumb_ld_rr_ri_enc<bits<3> reg_opc, bits<4> imm_opc,
                              Operand AddrMode_r, Operand AddrMode_i,
                              AddrMode am, InstrItinClass itin_r,
                              InstrItinClass itin_i, string asm,
                              PatFrag opnode> {
  def r : // reg/reg
    T1pILdStEncode<reg_opc,
                   (outs tGPR:$Rt), (ins AddrMode_r:$addr),
                   am, itin_r, asm, "\t$Rt, $addr",
                   [(set tGPR:$Rt, (opnode AddrMode_r:$addr))]>;
  def i : // reg/imm5
    T1pILdStEncodeImm<imm_opc, 1 /* Load */,
                      (outs tGPR:$Rt), (ins AddrMode_i:$addr),
                      am, itin_i, asm, "\t$Rt, $addr",
                      [(set tGPR:$Rt, (opnode AddrMode_i:$addr))]>;
}
// Stores: reg/reg and reg/imm5
multiclass thumb_st_rr_ri_enc<bits<3> reg_opc, bits<4> imm_opc,
                              Operand AddrMode_r, Operand AddrMode_i,
                              AddrMode am, InstrItinClass itin_r,
                              InstrItinClass itin_i, string asm,
                              PatFrag opnode> {
  def r : // reg/reg
    T1pILdStEncode<reg_opc,
                   (outs), (ins tGPR:$Rt, AddrMode_r:$addr),
                   am, itin_r, asm, "\t$Rt, $addr",
                   [(opnode tGPR:$Rt, AddrMode_r:$addr)]>;
  def i : // reg/imm5
    T1pILdStEncodeImm<imm_opc, 0 /* Store */,
                      (outs), (ins tGPR:$Rt, AddrMode_i:$addr),
                      am, itin_i, asm, "\t$Rt, $addr",
                      [(opnode tGPR:$Rt, AddrMode_i:$addr)]>;
}

// A8.6.57 & A8.6.60
defm tLDR  : thumb_ld_rr_ri_enc<0b100, 0b0110, t_addrmode_rrs4,
                                t_addrmode_is4, AddrModeT1_4,
                                IIC_iLoad_r, IIC_iLoad_i, "ldr",
                                UnOpFrag<(load node:$Src)>>;

// A8.6.64 & A8.6.61
defm tLDRB : thumb_ld_rr_ri_enc<0b110, 0b0111, t_addrmode_rrs1,
                                t_addrmode_is1, AddrModeT1_1,
                                IIC_iLoad_bh_r, IIC_iLoad_bh_i, "ldrb",
                                UnOpFrag<(zextloadi8 node:$Src)>>;

// A8.6.76 & A8.6.73
defm tLDRH : thumb_ld_rr_ri_enc<0b101, 0b1000, t_addrmode_rrs2,
                                t_addrmode_is2, AddrModeT1_2,
                                IIC_iLoad_bh_r, IIC_iLoad_bh_i, "ldrh",
                                UnOpFrag<(zextloadi16 node:$Src)>>;

let AddedComplexity = 10 in
def tLDRSB :                    // A8.6.80
  T1pILdStEncode<0b011, (outs tGPR:$Rt), (ins t_addrmode_rr:$addr),
                 AddrModeT1_1, IIC_iLoad_bh_r,
                 "ldrsb", "\t$Rt, $addr",
                 [(set tGPR:$Rt, (sextloadi8 t_addrmode_rr:$addr))]>;

let AddedComplexity = 10 in
def tLDRSH :                    // A8.6.84
  T1pILdStEncode<0b111, (outs tGPR:$Rt), (ins t_addrmode_rr:$addr),
                 AddrModeT1_2, IIC_iLoad_bh_r,
                 "ldrsh", "\t$Rt, $addr",
                 [(set tGPR:$Rt, (sextloadi16 t_addrmode_rr:$addr))]>;

let canFoldAsLoad = 1 in
def tLDRspi : T1pIs<(outs tGPR:$Rt), (ins t_addrmode_sp:$addr), IIC_iLoad_i,
                    "ldr", "\t$Rt, $addr",
                    [(set tGPR:$Rt, (load t_addrmode_sp:$addr))]>,
              T1LdStSP<{1,?,?}> {
  bits<3> Rt;
  bits<8> addr;
  let Inst{10-8} = Rt;
  let Inst{7-0} = addr;
}

// Load tconstpool
// FIXME: Use ldr.n to work around a Darwin assembler bug.
let canFoldAsLoad = 1, isReMaterializable = 1, isCodeGenOnly = 1 in
def tLDRpci : T1pIs<(outs tGPR:$Rt), (ins t_addrmode_pc:$addr), IIC_iLoad_i,
                  "ldr", ".n\t$Rt, $addr",
                  [(set tGPR:$Rt, (load (ARMWrapper tconstpool:$addr)))]>,
              T1Encoding<{0,1,0,0,1,?}> {
  // A6.2 & A8.6.59
  bits<3> Rt;
  bits<8> addr;
  let Inst{10-8} = Rt;
  let Inst{7-0}  = addr;
}

// FIXME: Remove this entry when the above ldr.n workaround is fixed.
// For disassembly use only.
def tLDRpciDIS : T1pIs<(outs tGPR:$Rt), (ins t_addrmode_pc:$addr), IIC_iLoad_i,
                       "ldr", "\t$Rt, $addr",
                       [/* disassembly only */]>,
                 T1Encoding<{0,1,0,0,1,?}> {
  // A6.2 & A8.6.59
  bits<3> Rt;
  bits<8> addr;
  let Inst{10-8} = Rt;
  let Inst{7-0}  = addr;
}

// A8.6.194 & A8.6.192
defm tSTR  : thumb_st_rr_ri_enc<0b000, 0b0110, t_addrmode_rrs4,
                                t_addrmode_is4, AddrModeT1_4,
                                IIC_iStore_r, IIC_iStore_i, "str",
                                BinOpFrag<(store node:$LHS, node:$RHS)>>;

// A8.6.197 & A8.6.195
defm tSTRB : thumb_st_rr_ri_enc<0b010, 0b0111, t_addrmode_rrs1,
                                t_addrmode_is1, AddrModeT1_1,
                                IIC_iStore_bh_r, IIC_iStore_bh_i, "strb",
                                BinOpFrag<(truncstorei8 node:$LHS, node:$RHS)>>;

// A8.6.207 & A8.6.205
defm tSTRH : thumb_st_rr_ri_enc<0b001, 0b1000, t_addrmode_rrs2,
                               t_addrmode_is2, AddrModeT1_2,
                               IIC_iStore_bh_r, IIC_iStore_bh_i, "strh",
                               BinOpFrag<(truncstorei16 node:$LHS, node:$RHS)>>;


def tSTRspi : T1pIs<(outs), (ins tGPR:$Rt, t_addrmode_sp:$addr), IIC_iStore_i,
                    "str", "\t$Rt, $addr",
                    [(store tGPR:$Rt, t_addrmode_sp:$addr)]>,
              T1LdStSP<{0,?,?}> {
  bits<3> Rt;
  bits<8> addr;
  let Inst{10-8} = Rt;
  let Inst{7-0} = addr;
}


//===----------------------------------------------------------------------===//
//  Arithmetic Instructions.
//

// Helper classes for encoding T1pI patterns:
class T1pIDPEncode<bits<4> opA, dag oops, dag iops, InstrItinClass itin,
                   string opc, string asm, list<dag> pattern>
    : T1pI<oops, iops, itin, opc, asm, pattern>,
      T1DataProcessing<opA> {
  bits<3> Rm;
  bits<3> Rn;
  let Inst{5-3} = Rm;
  let Inst{2-0} = Rn;
}
class T1pIMiscEncode<bits<7> opA, dag oops, dag iops, InstrItinClass itin,
                     string opc, string asm, list<dag> pattern>
    : T1pI<oops, iops, itin, opc, asm, pattern>,
      T1Misc<opA> {
  bits<3> Rm;
  bits<3> Rd;
  let Inst{5-3} = Rm;
  let Inst{2-0} = Rd;
}

// Helper classes for encoding T1sI patterns:
class T1sIDPEncode<bits<4> opA, dag oops, dag iops, InstrItinClass itin,
                   string opc, string asm, list<dag> pattern>
    : T1sI<oops, iops, itin, opc, asm, pattern>,
      T1DataProcessing<opA> {
  bits<3> Rd;
  bits<3> Rn;
  let Inst{5-3} = Rn;
  let Inst{2-0} = Rd;
}
class T1sIGenEncode<bits<5> opA, dag oops, dag iops, InstrItinClass itin,
                    string opc, string asm, list<dag> pattern>
    : T1sI<oops, iops, itin, opc, asm, pattern>,
      T1General<opA> {
  bits<3> Rm;
  bits<3> Rn;
  bits<3> Rd;
  let Inst{8-6} = Rm;
  let Inst{5-3} = Rn;
  let Inst{2-0} = Rd;
}
class T1sIGenEncodeImm<bits<5> opA, dag oops, dag iops, InstrItinClass itin,
                       string opc, string asm, list<dag> pattern>
    : T1sI<oops, iops, itin, opc, asm, pattern>,
      T1General<opA> {
  bits<3> Rd;
  bits<3> Rm;
  let Inst{5-3} = Rm;
  let Inst{2-0} = Rd;
}

// Helper classes for encoding T1sIt patterns:
class T1sItDPEncode<bits<4> opA, dag oops, dag iops, InstrItinClass itin,
                    string opc, string asm, list<dag> pattern>
    : T1sIt<oops, iops, itin, opc, asm, pattern>,
      T1DataProcessing<opA> {
  bits<3> Rdn;
  bits<3> Rm;
  let Inst{5-3} = Rm;
  let Inst{2-0} = Rdn;
}
class T1sItGenEncodeImm<bits<5> opA, dag oops, dag iops, InstrItinClass itin,
                        string opc, string asm, list<dag> pattern>
    : T1sIt<oops, iops, itin, opc, asm, pattern>,
      T1General<opA> {
  bits<3> Rdn;
  bits<8> imm8;
  let Inst{10-8} = Rdn;
  let Inst{7-0}  = imm8;
}

// Add with carry register
let isCommutable = 1, Uses = [CPSR] in
def tADC :                      // A8.6.2
  T1sItDPEncode<0b0101, (outs tGPR:$Rdn), (ins tGPR:$Rn, tGPR:$Rm), IIC_iALUr,
                "adc", "\t$Rdn, $Rm",
                [(set tGPR:$Rdn, (adde tGPR:$Rn, tGPR:$Rm))]>;

// Add immediate
def tADDi3 :                    // A8.6.4 T1
  T1sIGenEncodeImm<0b01110, (outs tGPR:$Rd), (ins tGPR:$Rm, imm0_7:$imm3),
                   IIC_iALUi,
                   "add", "\t$Rd, $Rm, $imm3",
                   [(set tGPR:$Rd, (add tGPR:$Rm, imm0_7:$imm3))]> {
  bits<3> imm3;
  let Inst{8-6} = imm3;
}

def tADDi8 :                    // A8.6.4 T2
  T1sItGenEncodeImm<{1,1,0,?,?}, (outs tGPR:$Rdn),
                    (ins tGPR:$Rn, imm0_255:$imm8), IIC_iALUi,
                    "add", "\t$Rdn, $imm8",
                    [(set tGPR:$Rdn, (add tGPR:$Rn, imm8_255:$imm8))]>;

// Add register
let isCommutable = 1 in
def tADDrr :                    // A8.6.6 T1
  T1sIGenEncode<0b01100, (outs tGPR:$Rd), (ins tGPR:$Rn, tGPR:$Rm),
                IIC_iALUr,
                "add", "\t$Rd, $Rn, $Rm",
                [(set tGPR:$Rd, (add tGPR:$Rn, tGPR:$Rm))]>;

let neverHasSideEffects = 1 in
def tADDhirr : T1pIt<(outs GPR:$Rdn), (ins GPR:$Rn, GPR:$Rm), IIC_iALUr,
                     "add", "\t$Rdn, $Rm", []>,
               T1Special<{0,0,?,?}> {
  // A8.6.6 T2
  bits<4> Rdn;
  bits<4> Rm;
  let Inst{7}   = Rdn{3};
  let Inst{6-3} = Rm;
  let Inst{2-0} = Rdn{2-0};
}

// AND register
let isCommutable = 1 in
def tAND :                      // A8.6.12
  T1sItDPEncode<0b0000, (outs tGPR:$Rdn), (ins tGPR:$Rn, tGPR:$Rm),
                IIC_iBITr,
                "and", "\t$Rdn, $Rm",
                [(set tGPR:$Rdn, (and tGPR:$Rn, tGPR:$Rm))]>;

// ASR immediate
def tASRri :                    // A8.6.14
  T1sIGenEncodeImm<{0,1,0,?,?}, (outs tGPR:$Rd), (ins tGPR:$Rm, imm_sr:$imm5),
                   IIC_iMOVsi,
                   "asr", "\t$Rd, $Rm, $imm5",
                   [(set tGPR:$Rd, (sra tGPR:$Rm, (i32 imm_sr:$imm5)))]> {
  bits<5> imm5;
  let Inst{10-6} = imm5;
}

// ASR register
def tASRrr :                    // A8.6.15
  T1sItDPEncode<0b0100, (outs tGPR:$Rdn), (ins tGPR:$Rn, tGPR:$Rm),
                IIC_iMOVsr,
                "asr", "\t$Rdn, $Rm",
                [(set tGPR:$Rdn, (sra tGPR:$Rn, tGPR:$Rm))]>;

// BIC register
def tBIC :                      // A8.6.20
  T1sItDPEncode<0b1110, (outs tGPR:$Rdn), (ins tGPR:$Rn, tGPR:$Rm),
                IIC_iBITr,
                "bic", "\t$Rdn, $Rm",
                [(set tGPR:$Rdn, (and tGPR:$Rn, (not tGPR:$Rm)))]>;

// CMN register
let isCompare = 1, Defs = [CPSR] in {
//FIXME: Disable CMN, as CCodes are backwards from compare expectations
//       Compare-to-zero still works out, just not the relationals
//def tCMN :                     // A8.6.33
//  T1pIDPEncode<0b1011, (outs), (ins tGPR:$lhs, tGPR:$rhs),
//               IIC_iCMPr,
//               "cmn", "\t$lhs, $rhs",
//               [(ARMcmp tGPR:$lhs, (ineg tGPR:$rhs))]>;

def tCMNz :                     // A8.6.33
  T1pIDPEncode<0b1011, (outs), (ins tGPR:$Rn, tGPR:$Rm),
               IIC_iCMPr,
               "cmn", "\t$Rn, $Rm",
               [(ARMcmpZ tGPR:$Rn, (ineg tGPR:$Rm))]>;

} // isCompare = 1, Defs = [CPSR]

// CMP immediate
let isCompare = 1, Defs = [CPSR] in {
def tCMPi8 : T1pI<(outs), (ins tGPR:$Rn, imm0_255:$imm8), IIC_iCMPi,
                  "cmp", "\t$Rn, $imm8",
                  [(ARMcmp tGPR:$Rn, imm0_255:$imm8)]>,
             T1General<{1,0,1,?,?}> {
  // A8.6.35
  bits<3> Rn;
  bits<8> imm8;
  let Inst{10-8} = Rn;
  let Inst{7-0}  = imm8;
}

// CMP register
def tCMPr :                     // A8.6.36 T1
  T1pIDPEncode<0b1010, (outs), (ins tGPR:$Rn, tGPR:$Rm),
               IIC_iCMPr,
               "cmp", "\t$Rn, $Rm",
               [(ARMcmp tGPR:$Rn, tGPR:$Rm)]>;

def tCMPhir : T1pI<(outs), (ins GPR:$Rn, GPR:$Rm), IIC_iCMPr,
                   "cmp", "\t$Rn, $Rm", []>,
              T1Special<{0,1,?,?}> {
  // A8.6.36 T2
  bits<4> Rm;
  bits<4> Rn;
  let Inst{7}   = Rn{3};
  let Inst{6-3} = Rm;
  let Inst{2-0} = Rn{2-0};
}
} // isCompare = 1, Defs = [CPSR]


// XOR register
let isCommutable = 1 in
def tEOR :                      // A8.6.45
  T1sItDPEncode<0b0001, (outs tGPR:$Rdn), (ins tGPR:$Rn, tGPR:$Rm),
                IIC_iBITr,
                "eor", "\t$Rdn, $Rm",
                [(set tGPR:$Rdn, (xor tGPR:$Rn, tGPR:$Rm))]>;

// LSL immediate
def tLSLri :                    // A8.6.88
  T1sIGenEncodeImm<{0,0,0,?,?}, (outs tGPR:$Rd), (ins tGPR:$Rm, imm0_31:$imm5),
                   IIC_iMOVsi,
                   "lsl", "\t$Rd, $Rm, $imm5",
                   [(set tGPR:$Rd, (shl tGPR:$Rm, (i32 imm:$imm5)))]> {
  bits<5> imm5;
  let Inst{10-6} = imm5;
}

// LSL register
def tLSLrr :                    // A8.6.89
  T1sItDPEncode<0b0010, (outs tGPR:$Rdn), (ins tGPR:$Rn, tGPR:$Rm),
                IIC_iMOVsr,
                "lsl", "\t$Rdn, $Rm",
                [(set tGPR:$Rdn, (shl tGPR:$Rn, tGPR:$Rm))]>;

// LSR immediate
def tLSRri :                    // A8.6.90
  T1sIGenEncodeImm<{0,0,1,?,?}, (outs tGPR:$Rd), (ins tGPR:$Rm, imm_sr:$imm5),
                   IIC_iMOVsi,
                   "lsr", "\t$Rd, $Rm, $imm5",
                   [(set tGPR:$Rd, (srl tGPR:$Rm, (i32 imm_sr:$imm5)))]> {
  bits<5> imm5;
  let Inst{10-6} = imm5;
}

// LSR register
def tLSRrr :                    // A8.6.91
  T1sItDPEncode<0b0011, (outs tGPR:$Rdn), (ins tGPR:$Rn, tGPR:$Rm),
                IIC_iMOVsr,
                "lsr", "\t$Rdn, $Rm",
                [(set tGPR:$Rdn, (srl tGPR:$Rn, tGPR:$Rm))]>;

// Move register
let isMoveImm = 1 in
def tMOVi8 : T1sI<(outs tGPR:$Rd), (ins imm0_255:$imm8), IIC_iMOVi,
                  "mov", "\t$Rd, $imm8",
                  [(set tGPR:$Rd, imm0_255:$imm8)]>,
             T1General<{1,0,0,?,?}> {
  // A8.6.96
  bits<3> Rd;
  bits<8> imm8;
  let Inst{10-8} = Rd;
  let Inst{7-0}  = imm8;
}
// Because we have an explicit tMOVSr below, we need an alias to handle
// the immediate "movs" form here. Blech.
def : InstAlias <"movs $Rdn, $imm",
                 (tMOVi8 tGPR:$Rdn, CPSR, imm0_255:$imm, 14, 0)>;

// A7-73: MOV(2) - mov setting flag.

let neverHasSideEffects = 1 in {
def tMOVr : Thumb1pI<(outs GPR:$Rd), (ins GPR:$Rm), AddrModeNone,
                      2, IIC_iMOVr,
                      "mov", "\t$Rd, $Rm", "", []>,
                  T1Special<{1,0,?,?}> {
  // A8.6.97
  bits<4> Rd;
  bits<4> Rm;
  let Inst{7}   = Rd{3};
  let Inst{6-3} = Rm;
  let Inst{2-0} = Rd{2-0};
}
let Defs = [CPSR] in
def tMOVSr      : T1I<(outs tGPR:$Rd), (ins tGPR:$Rm), IIC_iMOVr,
                      "movs\t$Rd, $Rm", []>, Encoding16 {
  // A8.6.97
  bits<3> Rd;
  bits<3> Rm;
  let Inst{15-6} = 0b0000000000;
  let Inst{5-3}  = Rm;
  let Inst{2-0}  = Rd;
}
} // neverHasSideEffects

// Multiply register
let isCommutable = 1 in
def tMUL :                      // A8.6.105 T1
  Thumb1sI<(outs tGPR:$Rd), (ins tGPR:$Rn, tGPR:$Rm), AddrModeNone, 2,
           IIC_iMUL32, "mul", "\t$Rd, $Rn, $Rm", "$Rm = $Rd",
           [(set tGPR:$Rd, (mul tGPR:$Rn, tGPR:$Rm))]>,
      T1DataProcessing<0b1101> {
  bits<3> Rd;
  bits<3> Rn;
  let Inst{5-3} = Rn;
  let Inst{2-0} = Rd;
  let AsmMatchConverter = "cvtThumbMultiply";
}

def :InstAlias<"mul${s}${p} $Rdm, $Rn", (tMUL tGPR:$Rdm, s_cc_out:$s, tGPR:$Rn,
                                               pred:$p)>;

// Move inverse register
def tMVN :                      // A8.6.107
  T1sIDPEncode<0b1111, (outs tGPR:$Rd), (ins tGPR:$Rn), IIC_iMVNr,
               "mvn", "\t$Rd, $Rn",
               [(set tGPR:$Rd, (not tGPR:$Rn))]>;

// Bitwise or register
let isCommutable = 1 in
def tORR :                      // A8.6.114
  T1sItDPEncode<0b1100, (outs tGPR:$Rdn), (ins tGPR:$Rn, tGPR:$Rm),
                IIC_iBITr,
                "orr", "\t$Rdn, $Rm",
                [(set tGPR:$Rdn, (or tGPR:$Rn, tGPR:$Rm))]>;

// Swaps
def tREV :                      // A8.6.134
  T1pIMiscEncode<{1,0,1,0,0,0,?}, (outs tGPR:$Rd), (ins tGPR:$Rm),
                 IIC_iUNAr,
                 "rev", "\t$Rd, $Rm",
                 [(set tGPR:$Rd, (bswap tGPR:$Rm))]>;

def tREV16 :                    // A8.6.135
  T1pIMiscEncode<{1,0,1,0,0,1,?}, (outs tGPR:$Rd), (ins tGPR:$Rm),
                 IIC_iUNAr,
                 "rev16", "\t$Rd, $Rm",
             [(set tGPR:$Rd, (rotr (bswap tGPR:$Rm), (i32 16)))]>;

def tREVSH :                    // A8.6.136
  T1pIMiscEncode<{1,0,1,0,1,1,?}, (outs tGPR:$Rd), (ins tGPR:$Rm),
                 IIC_iUNAr,
                 "revsh", "\t$Rd, $Rm",
                 [(set tGPR:$Rd, (sra (bswap tGPR:$Rm), (i32 16)))]>;

// Rotate right register
def tROR :                      // A8.6.139
  T1sItDPEncode<0b0111, (outs tGPR:$Rdn), (ins tGPR:$Rn, tGPR:$Rm),
                IIC_iMOVsr,
                "ror", "\t$Rdn, $Rm",
                [(set tGPR:$Rdn, (rotr tGPR:$Rn, tGPR:$Rm))]>;

// Negate register
def tRSB :                      // A8.6.141
  T1sIDPEncode<0b1001, (outs tGPR:$Rd), (ins tGPR:$Rn),
               IIC_iALUi,
               "rsb", "\t$Rd, $Rn, #0",
               [(set tGPR:$Rd, (ineg tGPR:$Rn))]>;

def : InstAlias<"neg${s}${p} $Rd, $Rm",
                 (tRSB tGPR:$Rd, s_cc_out:$s, tGPR:$Rm, pred:$p)>;

// Subtract with carry register
let Uses = [CPSR] in
def tSBC :                      // A8.6.151
  T1sItDPEncode<0b0110, (outs tGPR:$Rdn), (ins tGPR:$Rn, tGPR:$Rm),
                IIC_iALUr,
                "sbc", "\t$Rdn, $Rm",
                [(set tGPR:$Rdn, (sube tGPR:$Rn, tGPR:$Rm))]>;

// Subtract immediate
def tSUBi3 :                    // A8.6.210 T1
  T1sIGenEncodeImm<0b01111, (outs tGPR:$Rd), (ins tGPR:$Rm, imm0_7:$imm3),
                   IIC_iALUi,
                   "sub", "\t$Rd, $Rm, $imm3",
                   [(set tGPR:$Rd, (add tGPR:$Rm, imm0_7_neg:$imm3))]> {
  bits<3> imm3;
  let Inst{8-6} = imm3;
}

def tSUBi8 :                    // A8.6.210 T2
  T1sItGenEncodeImm<{1,1,1,?,?}, (outs tGPR:$Rdn),
                    (ins tGPR:$Rn, imm0_255:$imm8), IIC_iALUi,
                    "sub", "\t$Rdn, $imm8",
                    [(set tGPR:$Rdn, (add tGPR:$Rn, imm8_255_neg:$imm8))]>;

// Subtract register
def tSUBrr :                    // A8.6.212
  T1sIGenEncode<0b01101, (outs tGPR:$Rd), (ins tGPR:$Rn, tGPR:$Rm),
                IIC_iALUr,
                "sub", "\t$Rd, $Rn, $Rm",
                [(set tGPR:$Rd, (sub tGPR:$Rn, tGPR:$Rm))]>;

// Sign-extend byte
def tSXTB :                     // A8.6.222
  T1pIMiscEncode<{0,0,1,0,0,1,?}, (outs tGPR:$Rd), (ins tGPR:$Rm),
                 IIC_iUNAr,
                 "sxtb", "\t$Rd, $Rm",
                 [(set tGPR:$Rd, (sext_inreg tGPR:$Rm, i8))]>;

// Sign-extend short
def tSXTH :                     // A8.6.224
  T1pIMiscEncode<{0,0,1,0,0,0,?}, (outs tGPR:$Rd), (ins tGPR:$Rm),
                 IIC_iUNAr,
                 "sxth", "\t$Rd, $Rm",
                 [(set tGPR:$Rd, (sext_inreg tGPR:$Rm, i16))]>;

// Test
let isCompare = 1, isCommutable = 1, Defs = [CPSR] in
def tTST :                      // A8.6.230
  T1pIDPEncode<0b1000, (outs), (ins tGPR:$Rn, tGPR:$Rm), IIC_iTSTr,
               "tst", "\t$Rn, $Rm",
               [(ARMcmpZ (and_su tGPR:$Rn, tGPR:$Rm), 0)]>;

// Zero-extend byte
def tUXTB :                     // A8.6.262
  T1pIMiscEncode<{0,0,1,0,1,1,?}, (outs tGPR:$Rd), (ins tGPR:$Rm),
                 IIC_iUNAr,
                 "uxtb", "\t$Rd, $Rm",
                 [(set tGPR:$Rd, (and tGPR:$Rm, 0xFF))]>;

// Zero-extend short
def tUXTH :                     // A8.6.264
  T1pIMiscEncode<{0,0,1,0,1,0,?}, (outs tGPR:$Rd), (ins tGPR:$Rm),
                 IIC_iUNAr,
                 "uxth", "\t$Rd, $Rm",
                 [(set tGPR:$Rd, (and tGPR:$Rm, 0xFFFF))]>;

// Conditional move tMOVCCr - Used to implement the Thumb SELECT_CC operation.
// Expanded after instruction selection into a branch sequence.
let usesCustomInserter = 1 in  // Expanded after instruction selection.
  def tMOVCCr_pseudo :
  PseudoInst<(outs tGPR:$dst), (ins tGPR:$false, tGPR:$true, pred:$cc),
              NoItinerary,
             [/*(set tGPR:$dst, (ARMcmov tGPR:$false, tGPR:$true, imm:$cc))*/]>;

// tLEApcrel - Load a pc-relative address into a register without offending the
// assembler.

def tADR : T1I<(outs tGPR:$Rd), (ins t_adrlabel:$addr, pred:$p),
               IIC_iALUi, "adr{$p}\t$Rd, $addr", []>,
               T1Encoding<{1,0,1,0,0,?}> {
  bits<3> Rd;
  bits<8> addr;
  let Inst{10-8} = Rd;
  let Inst{7-0} = addr;
  let DecoderMethod = "DecodeThumbAddSpecialReg";
}

//===----------------------------------------------------------------------===//
// Non-Instruction Patterns
//

// Comparisons
def : Pat<(ARMcmpZ tGPR:$Rn, imm0_255:$imm8),
            (tCMPi8  tGPR:$Rn, imm0_255:$imm8)>;
def : Pat<(ARMcmpZ tGPR:$Rn, tGPR:$Rm),
            (tCMPr   tGPR:$Rn, tGPR:$Rm)>;

// Add with carry
def : Pat<(addc   tGPR:$lhs, imm0_7:$rhs),
            (tADDi3 tGPR:$lhs, imm0_7:$rhs)>;
def : Pat<(addc   tGPR:$lhs, imm8_255:$rhs),
            (tADDi8 tGPR:$lhs, imm8_255:$rhs)>;
def : Pat<(addc   tGPR:$lhs, tGPR:$rhs),
            (tADDrr tGPR:$lhs, tGPR:$rhs)>;

// Subtract with carry
def : Pat<(addc   tGPR:$lhs, imm0_7_neg:$rhs),
            (tSUBi3 tGPR:$lhs, imm0_7_neg:$rhs)>;
def : Pat<(addc   tGPR:$lhs, imm8_255_neg:$rhs),
            (tSUBi8 tGPR:$lhs, imm8_255_neg:$rhs)>;
def : Pat<(subc   tGPR:$lhs, tGPR:$rhs),
            (tSUBrr tGPR:$lhs, tGPR:$rhs)>;

// ConstantPool, GlobalAddress
def : Pat<(ARMWrapper  tglobaladdr :$dst), (tLEApcrel tglobaladdr :$dst)>;
def : Pat<(ARMWrapper  tconstpool  :$dst), (tLEApcrel tconstpool  :$dst)>;

// zextload i1 -> zextload i8
def : Pat<(zextloadi1 t_addrmode_rrs1:$addr),
            (tLDRBr t_addrmode_rrs1:$addr)>;
def : Pat<(zextloadi1 t_addrmode_is1:$addr),
            (tLDRBi t_addrmode_is1:$addr)>;

// extload -> zextload
def : Pat<(extloadi1  t_addrmode_rrs1:$addr), (tLDRBr t_addrmode_rrs1:$addr)>;
def : Pat<(extloadi1  t_addrmode_is1:$addr),  (tLDRBi t_addrmode_is1:$addr)>;
def : Pat<(extloadi8  t_addrmode_rrs1:$addr), (tLDRBr t_addrmode_rrs1:$addr)>;
def : Pat<(extloadi8  t_addrmode_is1:$addr),  (tLDRBi t_addrmode_is1:$addr)>;
def : Pat<(extloadi16 t_addrmode_rrs2:$addr), (tLDRHr t_addrmode_rrs2:$addr)>;
def : Pat<(extloadi16 t_addrmode_is2:$addr),  (tLDRHi t_addrmode_is2:$addr)>;

// If it's impossible to use [r,r] address mode for sextload, select to
// ldr{b|h} + sxt{b|h} instead.
def : Pat<(sextloadi8 t_addrmode_is1:$addr),
            (tSXTB (tLDRBi t_addrmode_is1:$addr))>;
def : Pat<(sextloadi8 t_addrmode_rrs1:$addr),
            (tSXTB (tLDRBr t_addrmode_rrs1:$addr))>;
def : Pat<(sextloadi16 t_addrmode_is2:$addr),
            (tSXTH (tLDRHi t_addrmode_is2:$addr))>;
def : Pat<(sextloadi16 t_addrmode_rrs2:$addr),
            (tSXTH (tLDRHr t_addrmode_rrs2:$addr))>;

def : Pat<(sextloadi8 t_addrmode_rrs1:$addr),
            (tASRri (tLSLri (tLDRBr t_addrmode_rrs1:$addr), 24), 24)>;
def : Pat<(sextloadi8 t_addrmode_is1:$addr),
            (tASRri (tLSLri (tLDRBi t_addrmode_is1:$addr), 24), 24)>;
def : Pat<(sextloadi16 t_addrmode_rrs2:$addr),
            (tASRri (tLSLri (tLDRHr t_addrmode_rrs2:$addr), 16), 16)>;
def : Pat<(sextloadi16 t_addrmode_is2:$addr),
            (tASRri (tLSLri (tLDRHi t_addrmode_is2:$addr), 16), 16)>;

def : Pat<(atomic_load_8 t_addrmode_is1:$src),
             (tLDRBi t_addrmode_is1:$src)>;
def : Pat<(atomic_load_8 t_addrmode_rrs1:$src),
             (tLDRBr t_addrmode_rrs1:$src)>;
def : Pat<(atomic_load_16 t_addrmode_is2:$src),
             (tLDRHi t_addrmode_is2:$src)>;
def : Pat<(atomic_load_16 t_addrmode_rrs2:$src),
             (tLDRHr t_addrmode_rrs2:$src)>;
def : Pat<(atomic_load_32 t_addrmode_is4:$src),
             (tLDRi t_addrmode_is4:$src)>;
def : Pat<(atomic_load_32 t_addrmode_rrs4:$src),
             (tLDRr t_addrmode_rrs4:$src)>;
def : Pat<(atomic_store_8 t_addrmode_is1:$ptr, tGPR:$val),
             (tSTRBi tGPR:$val, t_addrmode_is1:$ptr)>;
def : Pat<(atomic_store_8 t_addrmode_rrs1:$ptr, tGPR:$val),
             (tSTRBr tGPR:$val, t_addrmode_rrs1:$ptr)>;
def : Pat<(atomic_store_16 t_addrmode_is2:$ptr, tGPR:$val),
             (tSTRHi tGPR:$val, t_addrmode_is2:$ptr)>;
def : Pat<(atomic_store_16 t_addrmode_rrs2:$ptr, tGPR:$val),
             (tSTRHr tGPR:$val, t_addrmode_rrs2:$ptr)>;
def : Pat<(atomic_store_32 t_addrmode_is4:$ptr, tGPR:$val),
             (tSTRi tGPR:$val, t_addrmode_is4:$ptr)>;
def : Pat<(atomic_store_32 t_addrmode_rrs4:$ptr, tGPR:$val),
             (tSTRr tGPR:$val, t_addrmode_rrs4:$ptr)>;

// Large immediate handling.

// Two piece imms.
def : Pat<(i32 thumb_immshifted:$src),
            (tLSLri (tMOVi8 (thumb_immshifted_val imm:$src)),
                    (thumb_immshifted_shamt imm:$src))>;

def : Pat<(i32 imm0_255_comp:$src),
            (tMVN (tMOVi8 (imm_comp_XFORM imm:$src)))>;
