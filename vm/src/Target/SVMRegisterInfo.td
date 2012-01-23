/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

class SVMReg<bits<3> num, string n> : Register<n> {
    field bits<5> Num;
    let Namespace = "SVM";
    let Num = num;
}

def R0 : SVMReg< 0, "r0" >;
def R1 : SVMReg< 0, "r1" >;
def R2 : SVMReg< 0, "r2" >;
def R3 : SVMReg< 0, "r3" >;
def R4 : SVMReg< 0, "r4" >;
def R5 : SVMReg< 0, "r5" >;
def R6 : SVMReg< 0, "r6" >;
def R7 : SVMReg< 0, "r7" >;
