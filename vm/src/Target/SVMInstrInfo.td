/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

def SDT_SVMCall     : SDTypeProfile<0, 1, [SDTCisVT<0, i32>]>;
def call            : SDNode<"SVMISD::CALL", SDT_SVMCall, [SDNPHasChain]>;
def ret             : SDNode<"SVMISD::RETURN", SDTNone, [SDNPHasChain]>;

def bTarget : Operand<OtherVT>;
def callTarget : Operand<i32>;

/*
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

def RET : SVC<1, (outs), (ins),
 	"sys.ret", [(ret)]>;

def B : T18<(outs), (ins bTarget:$offset11),
    "b  \t$offset11", [(br bb:$offset11)]>;

def CALL : SVC<2, (outs), (ins callTarget:$offset11),
    "sys.call\t$offset11", [(call tglobaladdr:$offset11)]>;

def LDRsp : T11<1, (outs), (ins GPReg:$Rd, i8imm:$word8),
    "ldr\t$Rd, [SP, #$word8]", []>;

def STRsp : T11<0, (outs), (ins GPReg:$Rd, i8imm:$word8),
    "str\t$Rd, [SP, #$word8]", []>;
