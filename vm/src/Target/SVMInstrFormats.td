/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */


/****************************************************************
 * Base classes
 */

class InstSVM<dag outs, dag ins, string asmstr, list<dag> pattern> : Instruction
{
    let Namespace = "SVM";
    dag OutOperandList = outs;
    dag InOperandList = ins;
    let AsmString = asmstr;
    let Pattern = pattern;
}

class Pseudo<dag outs, dag ins, string asmstr, list<dag> pattern>
   : InstSVM<outs, ins, asmstr, pattern> {
   let isCodeGenOnly = 1;
   let isPseudo = 1;
   let Size = 0;
}

class ThumbInst16<dag outs, dag ins, string asmstr, list<dag> pattern>
    : InstSVM<outs, ins, asmstr, pattern>
{
    let Size = 2;
    field bits<16> Inst;
    bits<3> opA;
    let Inst{15-13} = opA;
}

class ThumbInst32<dag outs, dag ins, string asmstr, list<dag> pattern>
    : InstSVM<outs, ins, asmstr, pattern>
{
    let Size = 4;
    field bits<32> Inst;
    bits<16> firstWord;
    bits<16> secondWord;

    // Little endian
    let Inst{31-16} = secondWord;
    let Inst{15-0} = firstWord;

    bits<2> op1;
    bits<7> op2;
    let firstWord{15-13} = 0b111;
    let firstWord{12-11} = op1;
    let firstWord{10-4} = op2;
}


/****************************************************************
 * Patterns
 */

// Immediate ranges
def imm256_65535 : PatLeaf<(imm), [{
    return (unsigned)N->getZExtValue() <= 65535
        && (unsigned)N->getZExtValue() >= 256;
}]>;

// Small immediates
def imm12 : PatLeaf<(imm), [{ return (unsigned)N->getZExtValue() < 4096; }]>;
def imm8 : PatLeaf<(imm), [{ return (unsigned)N->getZExtValue() < 256; }]>;
def imm5 : PatLeaf<(imm), [{ return (unsigned)N->getZExtValue() < 32; }]>;
def imm3 : PatLeaf<(imm), [{ return (unsigned)N->getZExtValue() < 8; }]>;

// Single-use operations
def and_su : PatFrag<(ops node:$lhs, node:$rhs), (and node:$lhs, node:$rhs),
    [{ return N->hasOneUse(); }]>;


/****************************************************************
 * 16-bit Thumb-1 Formats
 * (Named according to the ARM7TDMI manual)
 */

// Format 1: move shifted register
class T1<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ThumbInst16<outs, ins, asmstr, pattern>
{
    let opA = 0;
    bits<2> opB;
    bits<5> offset5;
    bits<3> Rs;
    bits<3> Rd;
    let Inst{12-11} = opB;
    let Inst{10-6} = offset5;
    let Inst{5-3} = Rs;
    let Inst{2-0} = Rd;

    let isAsCheapAsAMove = 1;
    let isReMaterializable = 1;
    let Defs = [CPSR];
}

class T2<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ThumbInst16<outs, ins, asmstr, pattern>
{
    let opA = 0;
    bits<1> imm;
    bits<1> opB;
    bits<3> Rn;
    bits<3> Rs;
    bits<3> Rd;
    let Inst{12-11} = 3;
    let Inst{10} = imm;
    let Inst{9} = opB;
    let Inst{8-6} = Rn;
    let Inst{5-3} = Rs;
    let Inst{2-0} = Rd;

    let isReMaterializable = 1;
    let Defs = [CPSR];
}

class T3<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ThumbInst16<outs, ins, asmstr, pattern>
{
    let opA = 1;
    bits<2> opB;
    bits<3> Rd;
    bits<8> offset8;
    let Inst{12-11} = opB;
    let Inst{10-8} = Rd;
    let Inst{7-0} = offset8;

    let isReMaterializable = 1;
    let Defs = [CPSR];
}

class T4<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ThumbInst16<outs, ins, asmstr, pattern>
{
    let opA = 2;
    bits<4> opB;
    bits<3> Rs;
    bits<3> Rn;
    let Inst{12-10} = 0;
    let Inst{9-6} = opB;
    let Inst{5-3} = Rs;
    let Inst{2-0} = Rn;

    let isReMaterializable = 1;
    let Defs = [CPSR];
}

class T6<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ThumbInst16<outs, ins, asmstr, pattern>
{
    bits<3> Rd;
    bits<8> word8;
    bits<10> offset10;
    let opA = 2;
    let word8 = offset10{9-2};
    let Inst{12-11} = 1;
    let Inst{10-8} = Rd;
    let Inst{7-0} = word8;

    let isReMaterializable = 1;
    let isAsCheapAsAMove = 1;
}

class T11<bits<1> isLoad, dag outs, dag ins, string asmstr, list<dag> pattern>
    : ThumbInst16<outs, ins, asmstr, pattern>
{
    bits<3> Rd;
    bits<8> word8;
    bits<10> offset10;
    let opA = 4;
    let word8 = offset10{9-2};
    let Inst{12} = 1;
    let Inst{11} = isLoad;
    let Inst{10-8} = Rd;
    let Inst{7-0} = word8;
}

class T12<bits<1> isSP, dag outs, dag ins, string asmstr, list<dag> pattern>
    : ThumbInst16<outs, ins, asmstr, pattern>
{
    bits<3> Rd;
    bits<8> word8;
    bits<10> offset10;
    let opA = 5;
    let word8 = offset10{9-2};
    let Inst{12} = 0;
    let Inst{11} = isSP;
    let Inst{10-8} = Rd;
    let Inst{7-0} = word8;
}

class T16<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ThumbInst16<outs, ins, asmstr, pattern>
{
    let opA = 6;
    bits<4> cc;
    bits<8> offset8;
    let Inst{12} = 1;
    let Inst{11-8} = cc;
    let Inst{7-0} = offset8;
}

class T17<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ThumbInst16<outs, ins, asmstr, pattern>
{
    let opA = 6;
    bits<8> value8;
    let Inst{12-8} = 0b11111;
    let Inst{7-0} = value8;
    
    // Syscalls are not required to preserve the base pointer
    let Defs = [BP];
}

class T18<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ThumbInst16<outs, ins, asmstr, pattern>
{
    let opA = 7;
    bits<11> offset11;
    let Inst{12-11} = 0b00;
    let Inst{10-0} = offset11;
}


/****************************************************************
 * 32-bit Thumb21 Formats
 */

// Load/store with immediate, base class
class T32_LDSTbase<bits<7> op2v, bits<3> bpReg, dag outs, dag ins, string asmstr, list<dag> pattern>
    : ThumbInst32<outs, ins, asmstr, pattern>
{
    bits<3> Rb;
    bits<3> Rt;
    bits<12> offset12;

    let op1 = 0b11;
    let op2 = op2v;

    let firstWord{3} = 1;    // Base pointer in high reg
    let firstWord{2-0} = bpReg;

    let secondWord{15} = 0;  // Source/dest in low reg
    let secondWord{14-12} = Rt;
    let secondWord{11-0} = offset12;
    
    let neverHasSideEffects = 1;

    // Not marked rematerializable, LD/ST is expensive relative to spilling.
    let isReMaterializable = 0;
    let isAsCheapAsAMove = 0;
}

// Integer divide
class T32_DIV<bits<1> isUnsigned, dag outs, dag ins, string asmstr, list<dag> pattern>
    : ThumbInst32<outs, ins, asmstr, pattern>
{
    bits<3> Rn;
    bits<3> Rd;
    bits<3> Rm;

    let op1 = 0b11;
    let op2{6-2} = 0b01110;
    let op2{1} = isUnsigned;
    let op2{0} = 0b1;

    let firstWord{3} = 0b1;
    let firstWord{2-0} = Rn;
    let secondWord{15-11} = 0b11110;
    let secondWord{10-8} = Rd;
    let secondWord{7-3} = 0b11110;
    let secondWord{2-0} = Rm;
}

// 16-bit immediate (MOVT/MOVW)
class T32_imm16<bits<6> op2v, dag outs, dag ins, string asmstr, list<dag> pattern>
    : ThumbInst32<outs, ins, asmstr, pattern>
{
    bits<16> value16;
    bits<3> Rd;

    let op1 = 0b10;
    let op2{6} = value16{11};
    let op2{5-0} = op2v;
    let firstWord{3-0} = value16{15-12};
    let secondWord{15} = 0;
    let secondWord{14-12} = value16{10-8};
    let secondWord{11} = 0;  // Dest in low reg
    let secondWord{10-8} = Rd;
    let secondWord{7-0} = value16{7-0};

    let isReMaterializable = 1;
    let isAsCheapAsAMove = 1;
}


/****************************************************************
 * SVC Formats
 */

class S_ind<dag outs, dag ins, string asmstr, list<dag> pattern>
    : T17<outs, ins, asmstr, pattern>
{
    // Indirect SVC instruction
    bits<9> addr9;
    let value8{7} = 0;
    let value8{6-0} = addr9{8-2};
}

class S_call<dag outs, dag ins, string asmstr, list<dag> pattern>
    : T17<outs, ins, asmstr, pattern>
{
    // Direct syscall #0-63
    bits<6> imm6;
    let value8{7-6} = 0b10;
    let value8{5-0} = imm6;
}

class S_subsp<dag outs, dag ins, string asmstr, list<dag> pattern>
    : T17<outs, ins, asmstr, pattern>
{
    // Validated subtract from SP
    bits<7> offset7;
    bits<5> imm5;
    let imm5 = offset7{6-2};
    let value8{7-5} = 0b110;
    let value8{4-0} = imm5;
}

class S_ptr<dag outs, dag ins, string asmstr>
    : T17<outs, ins, asmstr, []>
{
    // Validate pointer
    bits<3> Rb;
    bits<3> Rs;
    let value8{7-3} = 0b11100;
    let value8{2-0} = Rs;
    
    let isReMaterializable = 1;
    let neverHasSideEffects = 1;
}

class S_callr<bits<1> tail, dag outs, dag ins, string asmstr, list<dag> pattern>
    : T17<outs, ins, asmstr, pattern>
{
    // Indirect call
    bits<3> Rs;
    let value8{7-4} = 0b1111;
    let value8{3} = tail;
    let value8{2-0} = Rs;
}
