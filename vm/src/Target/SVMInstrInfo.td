/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

def SDT_SVMCall     : SDTypeProfile<0, -1, [SDTCisVT<0, i32>]>;
def call            : SDNode<"SVMISD::CALL", SDT_SVMCall,
                        [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;

def ret             : SDNode<"SVMISD::RETURN", SDTNone,
                        [SDNPHasChain, SDNPOptInGlue]>;

def bTarget : Operand<OtherVT>;
def callTarget : Operand<i32>;

/*
 * Normal instructions
 */

def MOV : T1<(outs GPReg:$Rd), (ins GPReg:$Rs),
    "mov\t$Rd, $Rs",
    [(set GPReg:$Rd, GPReg:$Rs)]>;

def RET : T1<(outs), (ins), "ret", [(ret)]>;

def B : T18<(outs), (ins bTarget:$offset11),
    "b\t$offset11", [(br bb:$offset11)]>;
