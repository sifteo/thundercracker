/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

class InstSVM<dag outs, dag ins, string asmstr, list<dag> pattern> : Instruction
{
    field bits<16> Inst;

    let Namespace = "SVM";

    dag OutOperandList = outs;
    dag InOperandList = ins;
    let AsmString   = asmstr;
    let Pattern = pattern;
}

class F1<dag outs, dag ins, string asmstr, list<dag> pattern>
    : InstSVM<outs, ins, asmstr, pattern>
{
    bits<6>  op6;
    bits<10> imm10;
    let Inst{15-10} = op6;
    let Inst{9-0}  = imm10;
}
