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

class SVMRegHigh<bits<3> num, string n> : SVMReg<n> {
    field bits<3> Num;
    let CostPerUse = 10;
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

// Base pointers
def BRO : SVMRegHigh< 0, "bro" >;
def BRW : SVMRegHigh< 1, "brw" >;

// Hidden registers
def SP : SVMReg< "sp" >;
def CPSR : SVMReg< "cpsr" >;

def CPSRReg : RegisterClass<"SVM", [i32], 32, (add CPSR)>;

// General purpose registers
def GPReg : RegisterClass<"SVM", [i32], 32, (sequence "R%u", 0, 7)>;
    
// Base pointer registers
def BPReg : RegisterClass<"SVM", [i32], 32, (add BRO, BRW)>;
def BPWriteReg : RegisterClass<"SVM", [i32], 32, (add BRW)>;
