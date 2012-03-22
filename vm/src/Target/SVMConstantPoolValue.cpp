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

#include "SVMConstantPoolValue.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/Constant.h"
#include "llvm/Constants.h"
#include "llvm/GlobalValue.h"
#include "llvm/Type.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdlib>
using namespace llvm;


SVMConstantPoolValue::SVMConstantPoolValue(Type *Ty,
                                           SVMCP::SVMCPKind kind,
                                           SVMCP::SVMCPModifier modifier)
    : MachineConstantPoolValue(Ty), Kind(kind), Modifier(modifier) {}

SVMConstantPoolValue::~SVMConstantPoolValue() {}

const char *SVMConstantPoolValue::getModifierText() const
{
    switch (Modifier) {
    default: llvm_unreachable("Unknown modifier!");
    case SVMCP::no_modifier: return "none";
    case SVMCP::LB:          return "lb";
    }
}

int SVMConstantPoolValue::getExistingMachineCPValue(MachineConstantPool *CP,
                                                    unsigned Alignment)
{
    assert(false && "Shouldn't be calling this directly!");
    return -1;
}

void
SVMConstantPoolValue::addSelectionDAGCSEId(FoldingSetNodeID &ID)
{
    assert(false && "SVMConstantPoolValue should never appear in SelectionDAG");
}

bool
SVMConstantPoolValue::hasSameValue(SVMConstantPoolValue *CPV)
{
    return CPV->Kind == Kind && CPV->Modifier == Modifier;
}

void SVMConstantPoolValue::dump() const
{
    errs() << "  " << *this;
}

void SVMConstantPoolValue::print(raw_ostream &O) const
{
    if (Modifier)
        O << "(" << getModifierText() << ")";
}

SVMConstantPoolMBB::SVMConstantPoolMBB(LLVMContext &C,
                                       const MachineBasicBlock *mbb,
                                       SVMCP::SVMCPModifier Modifier)
: SVMConstantPoolValue((Type*)Type::getInt32Ty(C),
    SVMCP::CPMachineBasicBlock, Modifier), MBB(mbb) {}

SVMConstantPoolMBB *SVMConstantPoolMBB::Create(LLVMContext &C,
                                               const MachineBasicBlock *mbb,
                                               SVMCP::SVMCPModifier Modifier)
{
    return new SVMConstantPoolMBB(C, mbb, Modifier);
}

int SVMConstantPoolMBB::getExistingMachineCPValue(MachineConstantPool *CP,
                                                  unsigned Alignment)
{
    unsigned AlignMask = Alignment - 1;
    const std::vector<MachineConstantPoolEntry> Constants = CP->getConstants();

    for (unsigned i = 0, e = Constants.size(); i != e; ++i) {
        if (Constants[i].isMachineConstantPoolEntry() &&
            (Constants[i].getAlignment() & AlignMask) == 0) {

            SVMConstantPoolValue *CPV =
                (SVMConstantPoolValue *)Constants[i].Val.MachineCPVal;        
            if (hasSameValue(CPV))
                return i;
        }
    }

    return -1;
}

bool SVMConstantPoolMBB::hasSameValue(SVMConstantPoolValue *CPV)
{
    const SVMConstantPoolMBB *PMBB = dyn_cast<SVMConstantPoolMBB>(CPV);
    return PMBB && PMBB->MBB == MBB && SVMConstantPoolValue::hasSameValue(CPV);
}

void SVMConstantPoolMBB::print(raw_ostream &O) const {
    SVMConstantPoolValue::print(O);
    O << MBB->getSymbol()->AliasedSymbol().getName();
}
