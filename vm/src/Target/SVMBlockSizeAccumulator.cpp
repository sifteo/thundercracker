/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMBlockSizeAccumulator.h"
#include "SVMTargetMachine.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineConstantPool.h"
using namespace llvm;


void SVMBlockSizeAccumulator::clear()
{
    InstrSizeTotal = 0;
    ConstSizeTotal = 0;
    InstrPrefixTotal = 0;
    InstrSuffixTotal = 0;
    ConstAlignment = 1;
    UsedCPI.clear();
}

unsigned SVMBlockSizeAccumulator::getByteCount() const
{
    uint32_t Size = 0;

    Size += InstrPrefixTotal;

    Size = RoundUpToAlignment(Size, SVMTargetMachine::getBundleSize());
    Size += InstrSizeTotal;

    Size += InstrSuffixTotal;
    
    Size = RoundUpToAlignment(Size, ConstAlignment);
    Size += ConstSizeTotal;

    return Size;
}

void SVMBlockSizeAccumulator::InstrAlign(unsigned A)
{
    InstrSizeTotal = RoundUpToAlignment(InstrSizeTotal, A);
}

void SVMBlockSizeAccumulator::AddInstr(unsigned bytes)
{    
    // Add an abstract instruction, with only a specific size.
    assert(bytes == 0 || bytes == 2 || bytes == 4);
    assert(bytes != 4 || (InstrSizeTotal & 3) == 0);
    InstrSizeTotal += bytes;
}

void SVMBlockSizeAccumulator::AddInstrPrefix(unsigned bytes)
{
    InstrPrefixTotal += bytes;
}

void SVMBlockSizeAccumulator::AddInstrSuffix(unsigned bytes)
{
    InstrSuffixTotal += bytes;
}

void SVMBlockSizeAccumulator::AddConstant(const TargetData &TD,
    const MachineConstantPoolEntry &CPE)
{
    AddConstant(TD.getTypeAllocSize(CPE.getType()), CPE.getAlignment());
}

void SVMBlockSizeAccumulator::AddConstant(unsigned bytes, unsigned align)
{
    // Update the required alignment of the entire pool
    ConstAlignment = std::max(ConstAlignment, align);

    // Align this constant
    // XXX: This is not really sufficient for handling heterogeneously
    //      sized constant pools, since it's sensitive to the order we
    //      discover constants in.
    ConstSizeTotal = RoundUpToAlignment(ConstSizeTotal, align);

    ConstSizeTotal += bytes;
}

void SVMBlockSizeAccumulator::AddConstantsForInstr(const MachineInstr *MI)
{
    const MachineFunction *MF = MI->getParent()->getParent();
    const MachineConstantPool *Pool = MF->getConstantPool();
    const TargetData *TD = MF->getTarget().getTargetData();
    const std::vector<MachineConstantPoolEntry> &Constants = Pool->getConstants();

    // Look for any new constants used by this instr
    for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
        const MachineOperand &OP = MI->getOperand(i);
        if (!OP.isCPI())
            continue;

        unsigned CPI = OP.getIndex();
        if (UsedCPI.count(CPI))
            continue;
        
        // Found a new constant
        UsedCPI.insert(CPI);
        AddConstant(*TD, Constants[CPI]);
    }
}

void SVMBlockSizeAccumulator::AddInstr(const MachineInstr *MI)
{
    // Add a specific instruction and all of its referenced constants
    AddInstr(MI->getDesc().getSize());
    AddConstantsForInstr(MI);
}
