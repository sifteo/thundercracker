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
    "ldr\t$Rd, [PC, #$offset10]", [(set GPReg:$Rd, tconstpool:$offset10)]>;

def MOVWi16 : T32_imm16<0b100100, (outs GPReg:$Rd), (ins i32imm:$value16),
    "movw\t$Rd, #$value16", [(set GPReg:$Rd, imm256_65535:$value16)]>;


/****************************************************************
 * Function Calls
 */

def SDT_SVMCall : SDTypeProfile<0, -1, [SDTCisVT<0, i32>]>;

def SVMCall : SDNode<"SVMISD::CALL", SDT_SVMCall,
    [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;
def SVMTailCall : SDNode<"SVMISD::TAIL_CALL", SDT_SVMCall,
    [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;
def SVMSys64Call : SDNode<"SVMISD::SYS64_CALL", SDT_SVMCall,
    [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;

def SVMRet : SDNode<"SVMISD::RETURN", SDTNone,
    [SDNPHasChain, SDNPOptInGlue]>;

def callTarget : Operand<i32>,
    ComplexPattern<iPTR, 1, "SelectCallTarget",
    [tglobaladdr, texternalsym], []> {
    let MIOperandInfo = (ops tconstpool);
}

def tailCallTarget : Operand<i32>,
    ComplexPattern<iPTR, 1, "SelectTailCallTarget",
    [tglobaladdr, texternalsym], []> {
    let MIOperandInfo = (ops tconstpool);
}

// Implicit adjustment encoded into a function's address.
// These are emitted during frame lowering, and converted to target-specific
// relocations that are passed to our ELF linker (SVMELFProgramWriter).
def FNSTACK : Pseudo<(outs), (ins i32imm:$amt), ".fnstack $amt", []>;

// "Relaxed" version of FNSTACK, for the fragment hack in SVMAsmBackend.
def FNSTACK_R : Pseudo<(outs), (ins i32imm:$amt), ".fnstack $amt", []>;

// Pseudo-op for splitting functions across multiple flash blocks
def SPLIT : Pseudo<(outs), (ins), ".split", []>;

def RET : S_ind<(outs), (ins), "s.ret", [(SVMRet)]> {
    let addr9 = 0;
    let isReturn = 1;
    let isTerminator = 1;
}

let isCall = 1, Defs = [R0, R1] in {

    def CALL : S_ind<(outs), (ins AbsCPIop:$addr9),
        "s.call\t$addr9", [(SVMCall callTarget:$addr9)]>;

    def CALLr : S_callr<0, (outs), (ins GPReg:$Rs),
        "s.callr\t$Rs", [(SVMCall GPReg:$Rs)]>;

    def SYS64_CALL : S_call<(outs), (ins i32imm:$imm6),
        "s.sys\t$imm6", [(SVMSys64Call imm:$imm6)]>; 

    let isReturn = 1, isTerminator = 1 in {

        def TAIL_CALL : S_ind<(outs), (ins AbsCPIop:$addr9),
            "s.tcall\t$addr9", [(SVMTailCall tailCallTarget:$addr9)]>;

        def TAIL_CALLr : S_callr<1, (outs), (ins GPReg:$Rs),
            "s.tcallr\t$Rs", [(SVMTailCall GPReg:$Rs)]>;
    }
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
def SDT_SVMCmov : SDTypeProfile<1, 4,
    [SDTCisSameAs<0, 1>, SDTCisSameAs<0, 2> ]>;

def SVMCmp : SDNode<"SVMISD::CMP", SDT_SVMCmp, [SDNPOutGlue]>;
def SVMBrcond : SDNode<"SVMISD::BRCOND", SDT_SVMBrcond,
    [SDNPHasChain, SDNPInGlue, SDNPOutGlue]>;
def SVMCmov : SDNode<"SVMISD::CMOV", SDT_SVMCmov,
    [SDNPInGlue, SDNPOutGlue]>;

// Should have pattern (SVMBrcond bb:$offset8, imm:$cc), but we have
// to do this programmatically so we can glue the CMP instruction properly.
def Bcc : T16<(outs), (ins bccTarget:$offset8, CCop:$cc, CPSRReg:$c),
    "b$cc\t$offset8", []> {
    let isBranch = 1;
    let isTerminator = 1;
    let Uses = [CPSR];
}

def B : T18<(outs), (ins bTarget:$offset11),
    "b  \t$offset11", [(br bb:$offset11)]> {
    let isBranch = 1;
    let isTerminator = 1;
}

// Long unconditional branch (via indirect svc)
def LB : S_ind<(outs), (ins AbsCPIop:$addr9),
    "s.lb\t$addr9", []> {
    let isBranch = 1;
    let isTerminator = 1;
}

def CMOV : Pseudo<(outs GPReg:$Rd),
    (ins GPReg:$T, GPReg:$F, CCop:$cc, CPSRReg:$c),
    "!CMOV$cc\t$Rd, $T, $F",
    [(set GPReg:$Rd, (SVMCmov GPReg:$T, GPReg:$F, imm:$cc, CPSRReg:$c))]> {
    let Defs = [CPSR];
    let usesCustomInserter = 1;
}


/****************************************************************
 * Data processing instructions
 */

def NOP : ThumbInst16<(outs), (ins), "nop", []> {
    let opA = 5;
    let Inst{12-0} = 0b1111100000000;
}

// Format 1

def MOVSr : T1<(outs GPReg:$Rd), (ins GPReg:$Rs),
    "movs\t$Rd, $Rs", []> {
    let opB = 0;
    let offset5 = 0;
    let neverHasSideEffects = 1;
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
    let isReMaterializable = 1;
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
    "mul\t$Rd, $Rn", [(set GPReg:$Rd, (mul GPReg:$Rn, GPReg:$Rs))]> {
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

// Format 5

def MOVr : T5<0b10, (outs GPReg:$Rd), (ins GPReg:$Rs),
    "mov\t$Rd, $Rs", []> {
    // Does not affect CPSR.
    let neverHasSideEffects = 1;
}

// 32-bit data processing instructions

def UDIVr : T32_DIV<0b1, (outs GPReg:$Rd), (ins GPReg:$Rn, GPReg:$Rm),
    "udiv\t$Rd, $Rn, $Rm", [(set GPReg:$Rd, (udiv GPReg:$Rn, GPReg:$Rm))]>;

def SDIVr : T32_DIV<0b0, (outs GPReg:$Rd), (ins GPReg:$Rn, GPReg:$Rm),
    "sdiv\t$Rd, $Rn, $Rm", [(set GPReg:$Rd, (sdiv GPReg:$Rn, GPReg:$Rm))]>;


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

def PTR : S_ptr<(outs BPReg:$Rb), (ins GPReg:$Rs), "s.ptr\t$Rs">;

class T32_LD<bits<7> op2v, string opstr>
    : T32_LDSTbase<op2v, 0, (outs GPReg:$Rt),
        (ins BPReg:$Rb, i32imm:$offset12),
        !strconcat(opstr, "\t$Rt, [$Rb, #$offset12]"), []> {
    let mayLoad = 1;
}

class T32_ST<bits<7> op2v, string opstr>
    : T32_LDSTbase<op2v, 1, (outs),
        (ins GPReg:$Rt, BPReg:$Rb, i32imm:$offset12),
        !strconcat(opstr, "\t$Rt, [$Rb, #$offset12]"), []> {
    let mayStore = 1;
}

// Basic load/store opcodes
def STRB   : T32_ST <0b0001000, "strb">;
def STRH   : T32_ST <0b0001010, "strh">;
def STR    : T32_ST <0b0001100, "str">;
def LDRB   : T32_LD <0b0001001, "ldrb">;
def LDRH   : T32_LD <0b0001011, "ldrh">;
def LDR    : T32_LD <0b0001101, "ldr">;
def LDRSB  : T32_LD <0b0011001, "ldrsb">;
def LDRSH  : T32_LD <0b0011011, "ldrsh">;

// Offset stores (RAM is implied, memory is always linear)
//
// XXX: These are disabled for now too, due to a subtle bug... In validation,
//      we need to make some basic assumptions about the pointer we're getting.
//      For example, we need to know whether it's RAM or Flash. With these patterns,
//      however, there's no guarantee of this. We might be matching part of a shared
//      subexpression in which the argument to (add) is not actually a valid pointer
//      at all. I hit this in Chroma's Game() constructor, for example, where a subtract
//      of 1024 was being factored out (and left as a 'subs' instruction) whereas a
//      subsequent add (of greater than 1024) was being folded into these stores.
//      This breaks when (pointer - 1024) is prior to the start of RAM.
//
//      To fix this properly, we'll need to be able to have some more rigorous
//      notion of a pointer + base address probably.
//
//      For future work, I saved an input file that reproduces this bug:
//        vm/doc/offset-store-bug.bc
/*
def : Pat< (store GPReg:$Rt, (add BP, imm12:$off)), (STR GPReg:$Rt, BP, imm12:$off)>;
def : Pat< (truncstorei16 GPReg:$Rt, (add BP, imm12:$off)), (STRH GPReg:$Rt, BP, imm12:$off)>;
def : Pat< (truncstorei8 GPReg:$Rt, (add BP, imm12:$off)), (STRB GPReg:$Rt, BP, imm12:$off)>;
*/

// Offset loads. Currently disabled, since these are not guaranteed
// to work correctly on nonlinear memory such as cached flash!
// XXX: Be smarter about identifying cases where it's safe to use these forms.
/*
def : Pat< (load (add BP, imm12:$off)), (LDR BP, imm12:$off)>;
def : Pat< (zextloadi16 (add BP, imm12:$off)), (LDRH BP, imm12:$off)>;
def : Pat< (zextloadi8 (add BP, imm12:$off)), (LDRB BP, imm12:$off)>;
def : Pat< (sextloadi16 (add BP, imm12:$off)), (LDRSH BP, imm12:$off)>;
def : Pat< (sextloadi8 (add BP, imm12:$off)), (LDRSB BP, imm12:$off)>;
def : Pat< (extloadi16 (add BP, imm12:$off)), (LDRH BP, imm12:$off)>;
def : Pat< (extloadi8 (add BP, imm12:$off)), (LDRB BP, imm12:$off)>;
*/

// Direct stores
def : Pat< (store GPReg:$Rt, BP), (STR GPReg:$Rt, BP, 0)>;
def : Pat< (truncstorei16 GPReg:$Rt, BP), (STRH GPReg:$Rt, BP, 0)>;
def : Pat< (truncstorei8 GPReg:$Rt, BP), (STRB GPReg:$Rt, BP, 0)>;

// Direct loads, with sign/zero/any extend
def : Pat< (load BP), (LDR BP, 0)>;
def : Pat< (zextloadi16 BP), (LDRH BP, 0)>;
def : Pat< (zextloadi8 BP), (LDRB BP, 0)>;
def : Pat< (sextloadi16 BP), (LDRSH BP, 0)>;
def : Pat< (sextloadi8 BP), (LDRSB BP, 0)>;
def : Pat< (extloadi16 BP), (LDRH BP, 0)>;
def : Pat< (extloadi8 BP), (LDRB BP, 0)>;


/****************************************************************
 * Stack operations
 */
 
// Pattern to convert frameindex -> (SP, offset)
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

// Dynamic stack allocation
let Uses = [SP], Defs = [SP] in {
    def SPADJ : S_subsp<(outs), (ins i32imm:$offset7), "s.spadj $offset7", []>;
    def SPADJi : S_ind<(outs), (ins AbsCPIop:$addr9), "s.spadji $addr9", []>;
}

// Long (SVC-assisted) stack loads/stores, used when dealing with huge stack frames
def STRspi : S_ind<(outs), (ins GPReg:$Rd, AbsCPIop:$addr9),
    "s.strspi $Rd, $addr9", []> {
    let Uses = [SP];
    let mayStore = 1;
}    
def LDRspi : S_ind<(outs GPReg:$Rd), (ins AbsCPIop:$addr9),
    "s.ldrspi $Rd, $addr9", []> {
    let Uses = [SP];
    let mayLoad = 1;
}
