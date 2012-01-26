/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/****************************************************************
 * Function Calls
 */

def SDT_SVMCall     : SDTypeProfile<0, -1, [SDTCisVT<0, i32>]>;
def SDT_SPCallSeqStart : SDCallSeqStart<[ SDTCisVT<0, i32> ]>;
def SDT_SPCallSeqEnd   : SDCallSeqEnd<[ SDTCisVT<0, i32>,
                                        SDTCisVT<1, i32> ]>;

def call            : SDNode<"SVMISD::CALL", SDT_SVMCall,
    [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;
def ret             : SDNode<"SVMISD::RETURN", SDTNone,
    [SDNPHasChain, SDNPOptInGlue]>;
def callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_SPCallSeqStart,
    [SDNPHasChain, SDNPOutGlue]>;
def callseq_end   : SDNode<"ISD::CALLSEQ_END",   SDT_SPCallSeqEnd,
    [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

def bTarget : Operand<OtherVT>;
def callTarget : Operand<i32>;

def ADJCALLSTACKDOWN : Pseudo<(outs), (ins i32imm:$amt),
    "!ADJCALLSTACKDOWN $amt", [(callseq_start timm:$amt)]>;
def ADJCALLSTACKUP : Pseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2),
    "!ADJCALLSTACKUP $amt1", [(callseq_end timm:$amt1, timm:$amt2)]>;

def RET : SVC<1, (outs), (ins), "sys.ret", [(ret)]>;

def CALL : SVC<2, (outs), (ins callTarget:$offset11),
    "sys.call\t$offset11", [(call tglobaladdr:$offset11)]> {
    let isCall = 1;
}


/****************************************************************
 * Normal instructions
 */

def MOV : T1<(outs GPReg:$Rd), (ins GPReg:$Rs),
    "mov\t$Rd, $Rs",
    [(set GPReg:$Rd, GPReg:$Rs)]>
{
    // XXX: This sets conditions codes, since it's really a LSL!
    let opB = 0;
    let offset5 = 0;
}

// XXX: Immediate is only 8-bit now, need literal pools...
def MOVi : T3<(outs GPReg:$Rd), (ins i32imm:$offset8),
    "mov\t$Rd, #$offset8", [(set GPReg:$Rd, imm:$offset8)]>;

def B : T18<(outs), (ins bTarget:$offset11),
    "b  \t$offset11", [(br bb:$offset11)]>;

def LDRsp : T11<1, (outs), (ins GPReg:$Rd, i8imm:$word8),
    "ldr\t$Rd, [SP, #$word8]", []>;

def STRsp : T11<0, (outs), (ins GPReg:$Rd, i8imm:$word8),
    "str\t$Rd, [SP, #$word8]", []>;
