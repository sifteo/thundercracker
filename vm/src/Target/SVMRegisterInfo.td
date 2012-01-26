/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

class SVMReg<string n> : Register<n> {
    let Namespace = "SVM";
}

class SVMRegLow<bits<3> num, string n> : SVMReg<n> {
    field bits<3> Num;
    let Num = num;
}

def R0 : SVMRegLow< 0, "r0" >;
def R1 : SVMRegLow< 1, "r1" >;
def R2 : SVMRegLow< 2, "r2" >;
def R3 : SVMRegLow< 3, "r3" >;
def R4 : SVMRegLow< 4, "r4" >;
def R5 : SVMRegLow< 5, "r5" >;
def R6 : SVMRegLow< 6, "r6" >;
def R7 : SVMRegLow< 7, "r7" >;

def SP : SVMReg< "sp" >;
def CPSR : SVMReg< "cpsr" >;

// General purpose registers
def GPReg : RegisterClass<"SVM", [i32], 32,
    (add R0, R1, R2, R3, R4, R5, R6, R7)>;
