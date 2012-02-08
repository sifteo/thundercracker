/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */


/****************************************************************
 * Constants
 */
 
def RelCPIop : Operand<i32> {
    let EncoderMethod = "getRelCPIOpValue";
}

def AbsCPIop : Operand<i32> {
    let EncoderMethod = "getAbsCPIOpValue";
}

def LDRpc : T6<(outs GPReg:$Rd), (ins RelCPIop:$offset10),
    "ldr\t$Rd, [PC, #$offset10]", [(set GPReg:$Rd, tconstpool:$offset10)]> {
    let isAsCheapAsAMove = 1;
}

def MOVWi16 : T32_imm16<0b100100, (outs GPReg:$Rd), (ins i32imm:$value16),
    "movw\t$Rd, #$value16", [(set GPReg:$Rd, imm256_65535:$value16)]>;


/****************************************************************
 * Function Calls
 */

def SDT_SVMCall : SDTypeProfile<0, -1, [SDTCisVT<0, i32>]>;
def SDT_SPCallSeqStart : SDCallSeqStart<[ SDTCisVT<0, i32> ]>;
def SDT_SPCallSeqEnd : SDCallSeqEnd<[ SDTCisVT<0, i32>, SDTCisVT<1, i32> ]>;

def SVMCall : SDNode<"SVMISD::CALL", SDT_SVMCall,
    [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;
def SVMTailCall : SDNode<"SVMISD::TAIL_CALL", SDT_SVMCall,
    [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;
def SVMSys64Call : SDNode<"SVMISD::SYS64_CALL", SDT_SVMCall,
    [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;

def SVMRet : SDNode<"SVMISD::RETURN", SDTNone,
    [SDNPHasChain, SDNPOptInGlue]>;

def callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_SPCallSeqStart,
    [SDNPHasChain, SDNPOutGlue]>;
def callseq_end : SDNode<"ISD::CALLSEQ_END",   SDT_SPCallSeqEnd,
    [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

def callTarget : Operand<i32>,
    ComplexPattern<iPTR, 1, "SelectCallTarget", [tglobaladdr], []> {
    let MIOperandInfo = (ops tconstpool);
}

def tailCallTarget : Operand<i32>,
    ComplexPattern<iPTR, 1, "SelectTailCallTarget", [tglobaladdr], []> {
    let MIOperandInfo = (ops tconstpool);
}

// Explicit stack adjustments
def ADJCALLSTACKDOWN : Pseudo<(outs), (ins i32imm:$amt),
    "!ADJCALLSTACKDOWN $amt", [(callseq_start timm:$amt)]>;
def ADJCALLSTACKUP : Pseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2),
    "!ADJCALLSTACKUP $amt1", [(callseq_end timm:$amt1, timm:$amt2)]>;

// Implicit adjustment encoded into a function's address.
// These are emitted during frame lowering, and converted to target-specific
// relocations that are passed to our ELF linker (SVMELFProgramWriter).
def FNSTACK : Pseudo<(outs), (ins i32imm:$amt), ".fnstack $amt", []>;

// "Relaxed" version of FNSTACK, for the fragment hack in SVMAsmBackend.
def FNSTACK_R : Pseudo<(outs), (ins i32imm:$amt), ".fnstack $amt", []>;
    
def RET : S_call<(outs), (ins), "s.ret", [(SVMRet)]> {
    // _SYS_ret() _SC(0)
    let imm6 = 0;
    let isReturn = 1;
    let isTerminator = 1;
}

def CALL : S_ind<(outs), (ins AbsCPIop:$offset11),
    "s.call\t$offset11", [(SVMCall callTarget:$offset11)]> {
    let isCall = 1;
}

def TAIL_CALL : S_ind<(outs), (ins AbsCPIop:$offset11),
    "s.tcall\t$offset11", [(SVMTailCall tailCallTarget:$offset11)]> {
    let isCall = 1;
    let isReturn = 1;
    let isTerminator = 1;
}

def CALLr : S_ind<(outs), (ins GPReg:$Rs),
    "s.call\t$Rs", [(SVMCall GPReg:$Rs)]> {
    let isCall = 1;
}

def TAIL_CALLr : S_ind<(outs), (ins GPReg:$Rs),
    "s.tcall\t$Rs", [(SVMTailCall GPReg:$Rs)]> {
    let isCall = 1;
    let isReturn = 1;
    let isTerminator = 1;
}

def SYS64_CALL : S_call<(outs), (ins i32imm:$imm6),
    "s.sys\t$imm6", [(SVMSys64Call imm:$imm6)]> {
    let isCall = 1;
}


/****************************************************************
 * Control Flow
 */
 
def CCop : Operand<i32> {
    let PrintMethod = "printCCOperand";
}

def bTarget : Operand<OtherVT> { 
    let EncoderMethod = "getBTargetOpValue";
}

def bccTarget : Operand<OtherVT> { 
    let EncoderMethod = "getBCCTargetOpValue";
}
 
def SDT_SVMCmp : SDTypeProfile<0, 2, [SDTCisSameAs<0, 1>]>;
def SDT_SVMBrcond : SDTypeProfile<0, 2,
    [SDTCisVT<0, OtherVT>, SDTCisVT<1, i32>]>;
def SDT_SVMCmov : SDTypeProfile<0, 3,
    [SDTCisSameAs<0, 1>, SDTCisVT<2, i32>]>;

def SVMCmp : SDNode<"SVMISD::CMP", SDT_SVMCmp, [SDNPOutGlue]>;
def SVMBrcond : SDNode<"SVMISD::BRCOND", SDT_SVMBrcond,
    [SDNPHasChain, SDNPInGlue, SDNPOutGlue]>;
def SVMCmov : SDNode<"SVMISD::CMOV", SDT_SVMCmov, []>;

// Should have pattern (SVMBrcond bb:$offset8, imm:$cc), but we have
// to do this programmatically so we can glue the CMP instruction properly.
def Bcc : T16<(outs), (ins bccTarget:$offset8, CCop:$cc, CPSRReg:$c),
    "b$cc\t$offset8", []> {
    let isBranch = 1;
    let isTerminator = 1;
}

def B : T18<(outs), (ins bTarget:$offset11),
    "b  \t$offset11", [(br bb:$offset11)]> {
    let isBranch = 1;
    let isTerminator = 1;
}

def CMOV : Pseudo<(outs GPReg:$Rd), (ins GPReg:$T, GPReg:$F, CCop:$cc),
    "!CMOV$cc\t$Rd, $T, $F",
    [(set GPReg:$Rd, (SVMCmov GPReg:$T, GPReg:$F, imm:$cc))]> {
    let Defs = [CPSR];
}


/****************************************************************
 * Data processing instructions
 */

// Format 1

def MOVSr : T1<(outs GPReg:$Rd), (ins GPReg:$Rs),
    "movs\t$Rd, $Rs", []> {
    let opB = 0;
    let offset5 = 0;
}

def LSLi5 : T1<(outs GPReg:$Rd), (ins GPReg:$Rs, i32imm:$offset5),
    "lsl\t$Rd, $Rs, #$offset5",
    [(set GPReg:$Rd, (shl GPReg:$Rs, imm5:$offset5))]> {
    let opB = 0;
}

def LSRi5 : T1<(outs GPReg:$Rd), (ins GPReg:$Rs, i32imm:$offset5),
    "lsr\t$Rd, $Rs, #$offset5",
    [(set GPReg:$Rd, (srl GPReg:$Rs, imm5:$offset5))]> {
    let opB = 1;
}

def ASRi5 : T1<(outs GPReg:$Rd), (ins GPReg:$Rs, i32imm:$offset5),
    "asr\t$Rd, $Rs, #$offset5",
    [(set GPReg:$Rd, (sra GPReg:$Rs, imm5:$offset5))]> {
    let opB = 2;
}

// Format 2

def SUBSi3 : T2<(outs GPReg:$Rd), (ins GPReg:$Rs, i32imm:$Rn),
    "subs\t$Rd, $Rs, #$Rn", [(set GPReg:$Rd, (sub GPReg:$Rs, imm3:$Rn))]> {
    let imm = 1;
    let opB = 1;
}

def SUBSr : T2<(outs GPReg:$Rd), (ins GPReg:$Rs, GPReg:$Rn),
    "subs\t$Rd, $Rs, $Rn", [(set GPReg:$Rd, (sub GPReg:$Rs, GPReg:$Rn))]> {
    let imm = 0;
    let opB = 1;
}

def ADDSi3 : T2<(outs GPReg:$Rd), (ins GPReg:$Rs, i32imm:$Rn),
    "adds\t$Rd, $Rs, #$Rn", [(set GPReg:$Rd, (add GPReg:$Rs, imm3:$Rn))]> {
    let imm = 1;
    let opB = 0;
}

def ADDSr : T2<(outs GPReg:$Rd), (ins GPReg:$Rs, GPReg:$Rn),
    "adds\t$Rd, $Rs, $Rn", [(set GPReg:$Rd, (add GPReg:$Rs, GPReg:$Rn))]> {
    let imm = 0;
    let opB = 0;
}

def : Pat<(subc GPReg:$Rs, GPReg:$Rn),  (SUBSr GPReg:$Rs, GPReg:$Rn)>;
def : Pat<(subc GPReg:$Rs, imm3:$Rn),   (SUBSi3 GPReg:$Rs, imm3:$Rn)>;
def : Pat<(addc GPReg:$Rs, GPReg:$Rn),  (ADDSr GPReg:$Rs, GPReg:$Rn)>;
def : Pat<(addc GPReg:$Rs, imm3:$Rn),   (ADDSi3 GPReg:$Rs, imm3:$Rn)>;

// Format 3

def MOVSi8 : T3<(outs GPReg:$Rd), (ins i32imm:$offset8),
    "movs\t$Rd, #$offset8", [(set GPReg:$Rd, imm8:$offset8)]> {
    let opB = 0;
    let isAsCheapAsAMove = 1;
}

def CMPi8 : T3<(outs), (ins GPReg:$Rd, i32imm:$offset8),
    "cmp\t$Rd, #$offset8", [(SVMCmp GPReg:$Rd, imm8:$offset8)]> {
    let opB = 1;
    let isCompare = 1;
}

def ADDSi8 : T3<(outs GPReg:$Rd), (ins GPReg:$Rs, i32imm:$offset8),
    "adds\t$Rd, #$offset8", [(set GPReg:$Rd, (add GPReg:$Rs, imm8:$offset8))]> {
    let opB = 2;
    let Constraints = "$Rs = $Rd";
}

def SUBSi8 : T3<(outs GPReg:$Rd), (ins GPReg:$Rs, i32imm:$offset8),
    "subs\t$Rd, #$offset8", [(set GPReg:$Rd, (sub GPReg:$Rs, imm8:$offset8))]> {
    let opB = 3;
    let Constraints = "$Rs = $Rd";
}

def : Pat<(subc GPReg:$Rs, imm8:$Rn),   (SUBSi8 GPReg:$Rs, imm8:$Rn)>;
def : Pat<(addc GPReg:$Rs, imm8:$Rn),   (ADDSi8 GPReg:$Rs, imm8:$Rn)>;

// Format 4

def ANDSr : T4<(outs GPReg:$Rd), (ins GPReg:$Rs, GPReg:$Rn),
    "and\t$Rd, $Rn", [(set GPReg:$Rd, (and GPReg:$Rs, GPReg:$Rn))]> {
    let opB = 0;
    let Constraints = "$Rs = $Rd";
}

def EORSr : T4<(outs GPReg:$Rd), (ins GPReg:$Rs, GPReg:$Rn),
    "eor\t$Rd, $Rn", [(set GPReg:$Rd, (xor GPReg:$Rs, GPReg:$Rn))]> {
    let opB = 1;
    let Constraints = "$Rs = $Rd";
}

def LSLr : T4<(outs GPReg:$Rd), (ins GPReg:$Rs, GPReg:$Rn),
    "lsl\t$Rd, $Rn", [(set GPReg:$Rd, (shl GPReg:$Rs, GPReg:$Rn))]> {
    let opB = 2;
    let Constraints = "$Rs = $Rd";
}

def LSRr : T4<(outs GPReg:$Rd), (ins GPReg:$Rs, GPReg:$Rn),
    "lsr\t$Rd, $Rn", [(set GPReg:$Rd, (srl GPReg:$Rs, GPReg:$Rn))]> {
    let opB = 3;
    let Constraints = "$Rs = $Rd";
}

def ASRr : T4<(outs GPReg:$Rd), (ins GPReg:$Rs, GPReg:$Rn),
    "asr\t$Rd, $Rn", [(set GPReg:$Rd, (sra GPReg:$Rs, GPReg:$Rn))]> {
    let opB = 4;
    let Constraints = "$Rs = $Rd";
}

def ADCr : T4<(outs GPReg:$Rd), (ins GPReg:$Rs, GPReg:$Rn),
    "adc\t$Rd, $Rn", [(set GPReg:$Rd, (adde GPReg:$Rs, GPReg:$Rn))]> {
    let opB = 5;
    let Uses = [CPSR];
    let Constraints = "$Rs = $Rd";
}

def SBCr : T4<(outs GPReg:$Rd), (ins GPReg:$Rs, GPReg:$Rn),
    "sbc\t$Rd, $Rn", [(set GPReg:$Rd, (sube GPReg:$Rs, GPReg:$Rn))]> {
    let opB = 6;
    let Uses = [CPSR];
    let Constraints = "$Rs = $Rd";
}

def RORr : T4<(outs GPReg:$Rd), (ins GPReg:$Rs, GPReg:$Rn),
    "ror\t$Rd, $Rn", [(set GPReg:$Rd, (rotr GPReg:$Rs, GPReg:$Rn))]> {
    let opB = 7;
    let Constraints = "$Rs = $Rd";
}

def TSTr : T4<(outs), (ins GPReg:$Rs, GPReg:$Rn),
    "tst\t$Rs, $Rn", [(SVMCmp (and_su GPReg:$Rs, GPReg:$Rn), 0)]> {
    let opB = 8;
    let isCompare = 1;
}

def NEGr : T4<(outs GPReg:$Rs), (ins GPReg:$Rn),
    "neg\t$Rs, $Rn", [(set GPReg:$Rs, (ineg GPReg:$Rn))]> {
    let opB = 9;
}

def CMPr : T4<(outs), (ins GPReg:$Rs, GPReg:$Rn),
    "cmp\t$Rs, $Rn", [(SVMCmp GPReg:$Rs, GPReg:$Rn)]> {
    let opB = 10;
    let isCompare = 1;
}

def CMNr : T4<(outs), (ins GPReg:$Rs, GPReg:$Rn),
    "cmn\t$Rs, $Rn", [(SVMCmp GPReg:$Rs, (ineg GPReg:$Rn))]> {
    let opB = 11;
    let isCompare = 1;
}

def ORRr : T4<(outs GPReg:$Rd), (ins GPReg:$Rs, GPReg:$Rn),
    "orr\t$Rd, $Rn", [(set GPReg:$Rd, (or GPReg:$Rs, GPReg:$Rn))]> {
    let opB = 12;
    let Constraints = "$Rs = $Rd";
}

def MULr : T4<(outs GPReg:$Rd), (ins GPReg:$Rs, GPReg:$Rn),
    "orr\t$Rd, $Rn", [(set GPReg:$Rd, (mul GPReg:$Rn, GPReg:$Rs))]> {
    let opB = 13;
    let Constraints = "$Rs = $Rd";
}

def BICr : T4<(outs GPReg:$Rd), (ins GPReg:$Rs, GPReg:$Rn),
    "bic\t$Rd, $Rn", [(set GPReg:$Rd, (and GPReg:$Rs, (not GPReg:$Rn)))]> {
    let opB = 14;
    let Constraints = "$Rs = $Rd";
}

def MVNr : T4<(outs GPReg:$Rs), (ins GPReg:$Rn),
    "mvn\t$Rs, $Rn", [(set GPReg:$Rs, (not GPReg:$Rn))]> {
    let opB = 15;
}


/****************************************************************
 * Memory operations
 */

// Wrapper for matching address operands
def SVMWrapper : SDNode<"SVMISD::WRAPPER", SDTIntUnaryOp>;

// Selector for addresses, converts them to const pool entries
def ldAddrTarget : Operand<i32>,
    ComplexPattern<iPTR, 1, "SelectLDAddrTarget", [tglobaladdr], []> {
    let MIOperandInfo = (ops tconstpool);
}

// Match wrapped and converted addresses, convert to LDRpc instruction
def : Pat<(SVMWrapper ldAddrTarget:$a), (LDRpc ldAddrTarget:$a)>;

// Base pointer validation, represented as a reg-to-reg copy
def MOVptr : S_ptr<(outs BPReg:$Rd), (ins GPReg:$Rs), "s.mov\t$Rd, $Rs", []>;

// Pseudo-ops for disallowed operations
def MOVrptr : Pseudo<(outs GPReg:$Rd), (ins BPReg:$Rs), "!MOV\t$Rd, $Rs", []>;

def STRi   : T32_STi<0b0001100, "str",   [(store GPReg:$Rt, (add BPWriteReg:$Rn, imm12:$offset12))]>;
def STR    : T32_ST <0b0001100, "str",   [(store GPReg:$Rt, BPWriteReg:$Rn)]>;
def STRHi  : T32_STi<0b0001010, "strh",  [(truncstorei16 GPReg:$Rt, (add BPWriteReg:$Rn, imm12:$offset12))]>;
def STRH   : T32_ST <0b0001010, "strh",  [(truncstorei16 GPReg:$Rt, BPWriteReg:$Rn)]>;
def STRBi  : T32_STi<0b0001000, "strb",  [(truncstorei8 GPReg:$Rt, (add BPWriteReg:$Rn, imm12:$offset12))]>;
def STRB   : T32_ST <0b0001000, "strb",  [(truncstorei8 GPReg:$Rt, BPWriteReg:$Rn)]>;

def LDRi   : T32_LDi<0b0001101, "ldr",   [(set GPReg:$Rt, (load (add BPReg:$Rn, imm12:$offset12)))]>;
def LDR    : T32_LD <0b0001101, "ldr",   [(set GPReg:$Rt, (load BPReg:$Rn))]>;
def LDRHi  : T32_LDi<0b0001011, "ldrh",  [(set GPReg:$Rt, (zextloadi16 (add BPReg:$Rn, imm12:$offset12)))]>;
def LDRH   : T32_LD <0b0001011, "ldrh",  [(set GPReg:$Rt, (zextloadi16 BPReg:$Rn))]>;
def LDRBi  : T32_LDi<0b0001001, "ldrb",  [(set GPReg:$Rt, (zextloadi8 (add BPReg:$Rn, imm12:$offset12)))]>;
def LDRB   : T32_LD <0b0001001, "ldrb",  [(set GPReg:$Rt, (zextloadi8 BPReg:$Rn))]>;

def LDRSHi : T32_LDi<0b0011011, "ldrsh", [(set GPReg:$Rt, (sextloadi16 (add BPReg:$Rn, imm12:$offset12)))]>;
def LDRSH  : T32_LD <0b0011011, "ldrsh", [(set GPReg:$Rt, (sextloadi16 BPReg:$Rn))]>;
def LDRSBi : T32_LDi<0b0011001, "ldrsb", [(set GPReg:$Rt, (sextloadi8 (add BPReg:$Rn, imm12:$offset12)))]>;
def LDRSB  : T32_LD <0b0011001, "ldrsb", [(set GPReg:$Rt, (sextloadi8 BPReg:$Rn))]>;


/****************************************************************
 * Stack operations
 */

def ADDRsp : Operand<i32>,
    ComplexPattern<iPTR, 2, "SelectAddrSP", [frameindex], []> {
    /*
     * SelectAddrSP gives us two separate MIOperands, one with a FrameIndex
     * and one with an immediate offset. The FrameIndex gets lowered very late
     * in code generation, and we at the very last minute (during encoding
     * or printing) combine the lowered FI with the offset.
     */
    let PrintMethod = "printAddrSPValue";
    let EncoderMethod = "getAddrSPValue";
    let MIOperandInfo = (ops GPReg:$base, i32imm:$offset10);
}

def LDRsp : T11<1, (outs GPReg:$Rd), (ins ADDRsp:$offset10),
    "ldr\t$Rd, [SP, #$offset10]",
    [(set GPReg:$Rd, (load ADDRsp:$offset10))]> {
    let Uses = [SP];
}

def STRsp : T11<0, (outs), (ins GPReg:$Rd, ADDRsp:$offset10),
    "str\t$Rd, [SP, #$offset10]",
    [(store GPReg:$Rd, ADDRsp:$offset10)]> {
    let Uses = [SP];
}

def ADDsp : T12<1, (outs GPReg:$Rd), (ins ADDRsp:$offset10),
    "add\t$Rd, SP, #$offset10", [(set GPReg:$Rd, ADDRsp:$offset10)]> {
    let Uses = [SP];
}

// Pseudo-ops for disallowed operations
def LDRBRsp : Pseudo<(outs BPReg:$Rd), (ins ADDRsp:$offset),
    "!LDRBR\t$Rd, [SP, #$offset]",
    [(set BPReg:$Rd, (load ADDRsp:$offset))]> {
    let Uses = [SP];
}
def STRBRsp : Pseudo<(outs), (ins BPReg:$Rd, ADDRsp:$offset),
    "!STRBR\t$Rd, [SP, #$offset]",
    [(store BPReg:$Rd, ADDRsp:$offset)]> {
    let Uses = [SP];
}
