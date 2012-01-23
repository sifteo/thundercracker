/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

def FooInstr : F1<
     (outs IntRegs:$dst), (ins IntRegs:$src),
     "foo $src, $dst",
     [(set IntRegs:$dst, (not IntRegs:$src))]>;
