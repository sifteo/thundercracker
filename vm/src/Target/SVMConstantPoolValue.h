/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 *
 * ------------------------------------------------------------------------
 *
 * This file was largely based on the ARMConstantPoolValue code from LLVM 3.0,
 * distributed under the University of Illinois Open Source License.
 * 
 * Copyright (c) 2003-2011 University of Illinois at Urbana-Champaign.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal with
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimers.
 *
 *     * Redistributions in binary form must reproduce the above copyright notice,
 *       this list of conditions and the following disclaimers in the
 *       documentation and/or other materials provided with the distribution.
 *
 *     * Neither the names of the LLVM Team, University of Illinois at
 *       Urbana-Champaign, nor the names of its contributors may be used to
 *       endorse or promote products derived from this Software without specific
 *       prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
 * SOFTWARE.
 */

#ifndef SVM_CONSTANTPOOLVALUE_H
#define SVM_CONSTANTPOOLVALUE_H

#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/Support/ErrorHandling.h"
#include <cstddef>

namespace llvm {

class LLVMContext;
class MachineBasicBlock;

namespace SVMCP {
    enum SVMCPKind {
        CPMachineBasicBlock
    };

    enum SVMCPModifier {
        no_modifier,
        LB,
    };
}

class SVMConstantPoolValue : public MachineConstantPoolValue {
    SVMCP::SVMCPKind Kind;
    SVMCP::SVMCPModifier Modifier;

protected:
    SVMConstantPoolValue(Type *Ty, SVMCP::SVMCPKind Kind,
        SVMCP::SVMCPModifier Modifier);

public:
    virtual ~SVMConstantPoolValue();

    SVMCP::SVMCPModifier getModifier() const { return Modifier; }
    const char *getModifierText() const;
    bool hasModifier() const { return Modifier != SVMCP::no_modifier; }

    bool isMachineBasicBlock() const{ return Kind == SVMCP::CPMachineBasicBlock; }

    virtual unsigned getRelocationInfo() const { return 2; }
    virtual int getExistingMachineCPValue(MachineConstantPool *CP,
                                          unsigned Alignment);
    virtual void addSelectionDAGCSEId(FoldingSetNodeID &ID);
    virtual bool hasSameValue(SVMConstantPoolValue *CPV);

    virtual void print(raw_ostream &O) const;
    void print(raw_ostream *O) const { if (O) print(*O); }
    void dump() const;

    static bool classof(const SVMConstantPoolValue *) { return true; }
};

inline raw_ostream &operator<<(raw_ostream &O, const SVMConstantPoolValue &V) {
    V.print(O);
    return O;
}


class SVMConstantPoolMBB : public SVMConstantPoolValue {
    const MachineBasicBlock *MBB; // Machine basic block.

    SVMConstantPoolMBB(LLVMContext &C, const MachineBasicBlock *mbb,
        SVMCP::SVMCPModifier Modifier);

public:
    static SVMConstantPoolMBB *Create(LLVMContext &C,
                                      const MachineBasicBlock *mbb,
                                      SVMCP::SVMCPModifier Modifier);

    const MachineBasicBlock *getMBB() const { return MBB; }

    virtual int getExistingMachineCPValue(MachineConstantPool *CP,
        unsigned Alignment);
    virtual bool hasSameValue(SVMConstantPoolValue *CPV);

    virtual void print(raw_ostream &O) const;

    static bool classof(const SVMConstantPoolValue *CPV) {
        return CPV->isMachineBasicBlock();
    }
    static bool classof(const SVMConstantPoolMBB *) { return true; }
};

} // End llvm namespace

#endif
