/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMBlockSizeAccumulator.h"
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
    ConstAlignment = 1;
    UsedCPI.clear();
}

unsigned SVMBlockSizeAccumulator::getByteCount() const
{
    return RoundUpToAlignment(InstrSizeTotal, ConstAlignment) + ConstSizeTotal;
}

void SVMBlockSizeAccumulator::InstrAlign(unsigned A)
{
    InstrSizeTotal = RoundUpToAlignment(InstrSizeTotal, A);
}

void SVMBlockSizeAccumulator::AddInstr(const MachineInstr *MI)
{
    const MachineFunction *MF = MI->getParent()->getParent();
    const MachineConstantPool *Pool = MF->getConstantPool();
    const TargetData *TD = MF->getTarget().getTargetData();
    const std::vector<MachineConstantPoolEntry> &Constants = Pool->getConstants();
    unsigned Size = MI->getDesc().getSize();

    // Verify instruction size and alignment
    assert(Size == 0 || Size == 2 || Size == 4);
    assert(Size != 4 || (InstrSizeTotal & 3) == 0);
    if (Size == 0)
        return;

    // Count this instruction
    InstrSizeTotal += Size;

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
        const MachineConstantPoolEntry &CPE = Constants[CPI];
        
        // Update the required alignment of the entire pool
        ConstAlignment = std::max(ConstAlignment, CPE.getAlignment());

        // Align this constant
        // XXX: This is not really sufficient for handling heterogeneously
        //      sized constant pools, since it's sensitive to the order we
        //      discover constants in.
        ConstSizeTotal = RoundUpToAlignment(ConstSizeTotal, CPE.getAlignment());

        // Count the CPE size itself
        ConstSizeTotal += TD->getTypeAllocSize(CPE.getType());
    }
}
