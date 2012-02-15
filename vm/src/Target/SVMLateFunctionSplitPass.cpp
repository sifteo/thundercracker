/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * This is our last-resort for dealing with functions that are larger than
 * one flash block. We prefer to use higher-level (and target
 * independent) transforms to reduce the size of functions, but those
 * can't know how large a function will be once it's fully compiled.
 *
 * This pass operates on already-aligned MachineFunctions, which we can
 * precisely measure the size of. Since earlier passes were responsible for
 * splitting functions at natural places, here we just blindly stuff as much
 * code into a single block as possible.
 *
 * We must walk through the function linearly, keeping an accurate count
 * of how many bytes it's spent on instructions, constant pool data, and
 * how many additional bytes we'll need for reworking branches if a split
 * does become necessary.
 *
 * Just before we would have overflowed a flash block with any one of these
 * kinds of data, this pass adds a SPLIT pseudo-instruction. By adding the
 * SPLITs and converting near branches to long branches, it becomes trivial
 * for later passes to allocate these functions into multiple blocks.
 */

#include "SVM.h"
#include "SVMTargetMachine.h"
#include "SVMBlockSizeAccumulator.h"
#include "SVMInstrInfo.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/Module.h"
#include "llvm/codeGen/MachineInstr.h"
#include "llvm/codeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include <vector>
using namespace llvm;

namespace {

    class SVMLateFunctionSplitPass : public MachineFunctionPass {
        SVMTargetMachine& TM;

    public:
        static char ID;
        explicit SVMLateFunctionSplitPass(SVMTargetMachine &tm)
            : MachineFunctionPass(ID), TM(tm) {}

        bool runOnMachineFunction(MachineFunction &MF);
        void releaseMemory();

        const char *getPassName() const {
            return "SVM late function splitter pass";
        }
        
    private:
        SVMBlockSizeAccumulator BSA;
        SmallSet<MachineBasicBlock*, 32> PriorBlockBB;
        SmallSet<MachineBasicBlock*, 32> CurrentBlockBB;
        bool Changed;
        
        void beginBlock();
        void visitBasicBlock(MachineBasicBlock &MBB);
        void visitInstrBundle(MachineBasicBlock &MBB,
            MachineBasicBlock::iterator &FirstI,
            MachineBasicBlock::iterator &EndI);
        void visitInstr(MachineBasicBlock &MBB,
            MachineBasicBlock::iterator &I);
            
        void splitBeforeInstruction(MachineBasicBlock &MBB,
            MachineBasicBlock::iterator &I);
        void rewriteBranch(MachineBasicBlock &MBB,
            MachineBasicBlock::iterator &I, MachineBasicBlock &TBB);
    };
  
    char SVMLateFunctionSplitPass::ID = 0;
}

FunctionPass *llvm::createSVMLateFunctionSplitPass(SVMTargetMachine &TM)
{
    return new SVMLateFunctionSplitPass(TM);
}

void SVMLateFunctionSplitPass::releaseMemory()
{
    BSA.clear();
    PriorBlockBB.clear();
    CurrentBlockBB.clear();
}

void SVMLateFunctionSplitPass::beginBlock()
{
    // Blocks start out (almost) empty
    BSA.clear();
    
    // Always save space for a long branch to link this with the next block
    BSA.AddConstant(4, 4);
    BSA.AddInstrSuffix(2);
}

bool SVMLateFunctionSplitPass::runOnMachineFunction(MachineFunction &MF)
{
    releaseMemory();
    beginBlock();
    Changed = false;

    for (MachineFunction::iterator MBB = MF.begin(); MBB != MF.end(); ++MBB)
        visitBasicBlock(*MBB);

    return Changed;
}

void SVMLateFunctionSplitPass::visitBasicBlock(MachineBasicBlock &MBB)
{
    unsigned bundleSize = TM.getBundleSize();

    // Remember every BB that we visit, so we can keep track of BBs
    // behind an already-split portion of the function.
    CurrentBlockBB.insert(&MBB);

    // All basic blocks must align to a bundle boundary
    BSA.InstrAlign(bundleSize);

    // We can only split on a bundle boundary, so visit a bundle at a time.
    MachineBasicBlock::iterator FirstI = MBB.begin();
    while (FirstI != MBB.end()) {
        MachineBasicBlock::iterator NextI = FirstI;
        unsigned Bytes = 0;
        
        do {
            Bytes += NextI->getDesc().getSize();
            assert(Bytes <= bundleSize);
            NextI++;
        } while (Bytes < bundleSize && NextI != MBB.end());
        
        visitInstrBundle(MBB, FirstI, NextI);
        FirstI = NextI;
    }
}

void SVMLateFunctionSplitPass::visitInstrBundle(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator &FirstI,
    MachineBasicBlock::iterator &EndI)
{
    // Visit each instruction, doing early branch rewriting and tracking fn size
    for (MachineBasicBlock::iterator I = FirstI; I != EndI; ++I)
        visitInstr(MBB, I);

    // If any instruction in this bundle put us over budget, split before
    // the whole bundle.
    if (BSA.getByteCount() >= TM.getBlockSize()) {
        splitBeforeInstruction(MBB, FirstI);

        // The new block begins with this bundle
        BSA.clear();
        for (MachineBasicBlock::iterator I = FirstI; I != EndI; ++I)
            visitInstr(MBB, I);
    }
}

void SVMLateFunctionSplitPass::visitInstr(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator &I)
{   
    if (SVMInstrInfo::isNearBranchOpcode(I->getOpcode())) {
        /*
         * Near branches may require additional work. At this point, we can
         * categorize a branch in one of three ways:
         *
         *   1. It points to a prior block. In this case, we can immediately
         *      rewrite it as a long branch.
         *
         *   2. It points to a prior BB in the current flash block. In this
         *      case, we're guaranteed that it can stay as a near branch.
         *
         *   3. It points to a future BB. We don't know whether there will be
         *      a split yet between us and the target, so we need to account
         *      for the potential space cost of a long branch.
         */

        // Branch target is always the first opcode
        MachineBasicBlock *TBB = I->getOperand(0).getMBB();

        if (PriorBlockBB.count(TBB)) {
            // (1) Rewrite it immediately
            rewriteBranch(MBB, I, *TBB);

        } else if (!CurrentBlockBB.count(TBB)) {
            // (3) May need to rewrite it later, save space for it.

            if (SVMInstrInfo::isCondNearBranchOpcode(I->getOpcode())) {
                // Conditional branches may use a 16-bit trampoline,
                // prefixed at the beginning of the block.
                BSA.AddInstrPrefix(2);
            }

            // All branches may need a 32-bit constant
            BSA.AddConstant(4, 4);
        }
    }

    BSA.AddInstr(I);
}

void SVMLateFunctionSplitPass::splitBeforeInstruction(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator &I)
{
    // Current BBs become prior
    PriorBlockBB.insert(CurrentBlockBB.begin(), CurrentBlockBB.end());
    CurrentBlockBB.clear();

    // Add the SPLIT instruction itself, telling future passes where to change blocks
    const TargetInstrInfo &TII = *TM.getInstrInfo();
    BuildMI(MBB, I, I->getDebugLoc(), TII.get(SVM::SPLIT));

    Changed = true;
}

void SVMLateFunctionSplitPass::rewriteBranch(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator &I, MachineBasicBlock &TBB)
{
}
