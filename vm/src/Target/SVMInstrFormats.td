/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

class InstSVM<dag outs, dag ins, string asmstr, list<dag> pattern> : Instruction
{
    let Namespace = "SVM";
    dag OutOperandList = outs;
    dag InOperandList = ins;
    let AsmString = asmstr;
    let Pattern = pattern;
    let Size = 2;
}

class Pseudo<dag outs, dag ins, string asmstr, list<dag> pattern>
   : InstSVM<outs, ins, asmstr, pattern> {
   let isCodeGenOnly = 1;
   let isPseudo = 1;
   let Size = 0;
}

/*
 * Patterns
 */

// Small immediates
def imm8 : PatLeaf<(imm), [{ return (unsigned)N->getZExtValue() < 256; }]>;
def imm5 : PatLeaf<(imm), [{ return (unsigned)N->getZExtValue() < 32; }]>;
def imm3 : PatLeaf<(imm), [{ return (unsigned)N->getZExtValue() < 8; }]>;

// Single-use operations
def and_su : PatFrag<(ops node:$lhs, node:$rhs), (and node:$lhs, node:$rhs),
    [{ return N->hasOneUse(); }]>;

/*
 * A 16-bit Thumb instruction. The top three bits are always a constant
 * "major" opcode. The individual formats here are numbered accoRding to
 * Figure 5-1 in the ARM7TDMI data sheet.
 */
 
class ThumbInst<dag outs, dag ins, string asmstr, list<dag> pattern>
    : InstSVM<outs, ins, asmstr, pattern>
{
    field bits<16> Inst;
    bits<3> opA;
    let Inst{15-13} = opA;
}

// Format 1: move shifted register
class T1<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ThumbInst<outs, ins, asmstr, pattern>
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
    let Defs = [CPSR];
}

class T2<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ThumbInst<outs, ins, asmstr, pattern>
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
    let Defs = [CPSR];
}

class T3<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ThumbInst<outs, ins, asmstr, pattern>
{
    let opA = 1;
    bits<2> opB;
    bits<3> Rd;
    bits<8> offset8;
    let Inst{12-11} = opB;
    let Inst{10-8} = Rd;
    let Inst{7-0} = offset8;
    let Defs = [CPSR];
}

class T4<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ThumbInst<outs, ins, asmstr, pattern>
{
    let opA = 2;
    bits<4> opB;
    bits<3> Rs;
    bits<3> Rn;
    let Inst{12-10} = 0;
    let Inst{9-6} = opB;
    let Inst{5-3} = Rs;
    let Inst{2-0} = Rn;
    let Defs = [CPSR];
}

class T11<bits<1> isLoad, dag outs, dag ins, string asmstr, list<dag> pattern>
    : ThumbInst<outs, ins, asmstr, pattern>
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
    : ThumbInst<outs, ins, asmstr, pattern>
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
    : ThumbInst<outs, ins, asmstr, pattern>
{
    let opA = 6;
    bits<4> cc;
    bits<8> offset8;
    let Inst{12} = 1;
    let Inst{11-8} = cc;
    let Inst{7-0} = offset8;
}

class T17<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ThumbInst<outs, ins, asmstr, pattern>
{
    let opA = 6;
    bits<8> value8;
    let Inst{12-8} = 0b11111;
    let Inst{7-0} = value8;
}

class T18<dag outs, dag ins, string asmstr, list<dag> pattern>
    : ThumbInst<outs, ins, asmstr, pattern>
{
    let opA = 7;
    bits<11> offset11;
    let Inst{12-11} = 0b00;
    let Inst{10-0} = offset11;
}

class SVC<bits<8> num, dag outs, dag ins, string asmstr, list<dag> pattern>
    : T17<outs, ins, asmstr, pattern>
{
    let value8 = num;
}
