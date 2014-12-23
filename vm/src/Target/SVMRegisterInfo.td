/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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

// Hidden registers
def CPSR : SVMReg< "cpsr" >;
def SP : SVMReg< "sp" >;
def BP : SVMReg< "bp" >;

// Minimal register classes for hidden registers
let isAllocatable = 0 in {
    def CPSRReg : RegisterClass<"SVM", [i32], 32, (add CPSR)>;
    def SPReg : RegisterClass<"SVM", [i32], 32, (add SP)>;
    def BPReg : RegisterClass<"SVM", [i32], 32, (add BP)>;
}

// General purpose registers
def GPReg : RegisterClass<"SVM", [i32], 32, (sequence "R%u", 0, 7)>;
