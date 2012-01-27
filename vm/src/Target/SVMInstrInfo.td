/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/****************************************************************
 * Function Calls
 */

def SDT_SVMCall : SDTypeProfile<0, -1, [SDTCisVT<0, i32>]>;
def SDT_SPCallSeqStart : SDCallSeqStart<[ SDTCisVT<0, i32> ]>;
def SDT_SPCallSeqEnd : SDCallSeqEnd<[ SDTCisVT<0, i32>, SDTCisVT<1, i32> ]>;

def SVMCall : SDNode<"SVMISD::CALL", SDT_SVMCall,
    [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;
def SVMRet : SDNode<"SVMISD::RETURN", SDTNone,
    [SDNPHasChain, SDNPOptInGlue]>;
def callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_SPCallSeqStart,
    [SDNPHasChain, SDNPOutGlue]>;
def callseq_end : SDNode<"ISD::CALLSEQ_END",   SDT_SPCallSeqEnd,
    [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

def bTarget : Operand<OtherVT>;
def callTarget : Operand<i32>;

def ADJCALLSTACKDOWN : Pseudo<(outs), (ins i32imm:$amt),
    "!ADJCALLSTACKDOWN $amt", [(callseq_start timm:$amt)]>;
def ADJCALLSTACKUP : Pseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2),
    "!ADJCALLSTACKUP $amt1", [(callseq_end timm:$amt1, timm:$amt2)]>;

def RET : SVC<1, (outs), (ins), "s.ret", [(SVMRet)]>;

def CALL : SVC<2, (outs), (ins callTarget:$offset11),
    "s.call\t$offset11", [(SVMCall tglobaladdr:$offset11)]> {
    let isCall = 1;
}

/****************************************************************
 * Conditionals
 */
 
def CCop : Operand<i32> {
    let PrintMethod = "printCCOperand";
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

def CMPr : T4<(outs), (ins GPReg:$Rd, GPReg:$Rs),
    "cmp\t$Rd, $Rs", [(SVMCmp GPReg:$Rd, GPReg:$Rs)]> {
    let isCompare = 1;
    let Defs = [CPSR];
}

def Bcc : T16<(outs), (ins bTarget:$offset11, CCop:$cc),
    "b$cc\t$offset11", [(SVMBrcond bb:$offset11, imm:$cc)]> {
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

def MOVSi : T3<(outs GPReg:$Rd), (ins i32imm:$offset8),
    "movs\t$Rd, #$offset8", [(set GPReg:$Rd, imm8:$offset8)]> {
    let opB = 0;
    let isAsCheapAsAMove = 1;
    let Defs = [CPSR];
}

def MOVSr : T1<(outs GPReg:$Rd), (ins GPReg:$Rs),
    "movs\t$Rd, $Rs", []> {
    let opB = 0;
    let offset5 = 0;
    let isAsCheapAsAMove = 1;
    let Defs = [CPSR];
}

def SUBSi : T2<(outs GPReg:$Rd), (ins GPReg:$Rs, i32imm:$Rn),
    "subs\t$Rd, $Rs, #$Rn", [(set GPReg:$Rd, (sub GPReg:$Rs, imm8:$Rn))]> {
    let imm = 1;
    let opB = 1;
    let Defs = [CPSR];
}

def SUBSr : T2<(outs GPReg:$Rd), (ins GPReg:$Rs, GPReg:$Rn),
    "subs\t$Rd, $Rs, $Rn", [(set GPReg:$Rd, (sub GPReg:$Rs, GPReg:$Rn))]> {
    let imm = 0;
    let opB = 1;
    let Defs = [CPSR];
}

def ADDSi : T2<(outs GPReg:$Rd), (ins GPReg:$Rs, i32imm:$Rn),
    "adds\t$Rd, $Rs, #$Rn", [(set GPReg:$Rd, (add GPReg:$Rs, imm8:$Rn))]> {
    let imm = 1;
    let opB = 0;
    let Defs = [CPSR];
}

def ADDSr : T2<(outs GPReg:$Rd), (ins GPReg:$Rs, GPReg:$Rn),
    "adds\t$Rd, $Rs, $Rn", [(set GPReg:$Rd, (add GPReg:$Rs, GPReg:$Rn))]> {
    let imm = 0;
    let opB = 0;
    let Defs = [CPSR];
}

def : Pat<(subc GPReg:$Rs, GPReg:$Rn),  (SUBSr GPReg:$Rs, GPReg:$Rn)>;
def : Pat<(subc GPReg:$Rs, imm8:$Rn),   (SUBSi GPReg:$Rs, imm8:$Rn)>;
def : Pat<(addc GPReg:$Rs, GPReg:$Rn),  (ADDSr GPReg:$Rs, GPReg:$Rn)>;
def : Pat<(addc GPReg:$Rs, imm8:$Rn),   (ADDSi GPReg:$Rs, imm8:$Rn)>;
  
def SBCr : T4<(outs GPReg:$Rd), (ins GPReg:$Rs, GPReg:$Rn),
    "sbc\t$Rd, $Rn", [(set GPReg:$Rd, (sube GPReg:$Rs, GPReg:$Rn))]> {
    let opB = 0b0110;
    let Uses = [CPSR];
    let Defs = [CPSR];
    let Constraints = "$Rs = $Rd";
}

def B : T18<(outs), (ins bTarget:$offset11),
    "b  \t$offset11", [(br bb:$offset11)]> {
    let isBranch = 1;
    let isTerminator = 1;
}


/****************************************************************
 * Stack operations
 */

def ADDRsp : Operand<i32>,
    ComplexPattern<iPTR, 2, "SelectAddrSP", [frameindex], []> {
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
