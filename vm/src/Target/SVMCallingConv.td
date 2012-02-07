/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

def CC_SVM : CallingConv<[
    CCIfType<[i8, i16], CCPromoteToType<i32>>,
    CCIfType<[i32, f32], CCAssignToReg<[R0, R1, R2, R3, R4, R5, R6, R7]>>,
    CCAssignToStack<4, 4>
]>;
