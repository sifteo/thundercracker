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
#include "SVMConstantPoolValue.h"
#include "SVMBlockSizeAccumulator.h"
#include "SVMInstrInfo.h"
#include "llvm/Module.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
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
        typedef std::vector<std::pair<MCSymbol*,MachineBasicBlock*> > AnonMBBFixupListTy;
        
        SVMBlockSizeAccumulator BSA;
        AnonMBBFixupListTy AnonMBBFixupList;
        unsigned FirstLocalBB;

        void beginBlock();
        
        MachineBasicBlock *createAnonymousMBB(MachineFunction *MF);
        void fixupAnonymousMBB(MachineFunction *MF);
        void setHasAddressTaken(MachineBasicBlock *MBB) const;

        bool visitBasicBlock(MachineFunction::iterator &MBB);
        bool visitInstrBundle(MachineFunction::iterator &MBB,
            MachineBasicBlock::iterator &FirstI,
            MachineBasicBlock::iterator &EndI);
        void visitInstr(MachineFunction::iterator &MBB,
            MachineBasicBlock::iterator &I);

        void splitBeforeInstruction(MachineFunction::iterator &MBB,
            MachineBasicBlock::iterator &I);

        void rewriteBranchesInRange(MachineFunction *MF, unsigned beginBB,
            unsigned endBB, MachineFunction::iterator insertPoint);
        void rewriteBranch(MachineBasicBlock *MBB,
            MachineBasicBlock::iterator &I, MachineBasicBlock &TBB,
            MachineFunction::iterator insertPoint);
            
        unsigned getLongBranchCPI(MachineBasicBlock *MBB) const;
    };
  
    char SVMLateFunctionSplitPass::ID = 0;
}

FunctionPass *llvm::createSVMLateFunctionSplitPass(SVMTargetMachine &TM)
{
    return new SVMLateFunctionSplitPass(TM);
}

void SVMLateFunctionSplitPass::releaseMemory()
{
    AnonMBBFixupList.clear();
    BSA.clear();
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
    bool Changed = false;

    // We require that BB numbers are sequential
    MF.RenumberBlocks();

    // At this point, assume that every MF begins on a flash block.
    // This is only necessarily true of multi-block functions, but it won't
    // do any harm if this function does turn out to be smaller than one block.
    beginBlock();
    FirstLocalBB = 0;

    // NB: visitBasicBlock() will update MBB appropriately
    for (MachineFunction::iterator MBB = MF.begin(); MBB != MF.end();)
        if (visitBasicBlock(MBB))
            Changed = true;

    // Rewrite branches in the last block, if necessary.
    rewriteBranchesInRange(&MF, FirstLocalBB, MF.getNumBlockIDs(), MF.end());

    // Fixup references to new anonymous MBBs
    fixupAnonymousMBB(&MF);

    return Changed;
}

bool SVMLateFunctionSplitPass::visitBasicBlock(MachineFunction::iterator &MBB)
{
    unsigned bundleSize = TM.getBundleSize();

    // All basic blocks must align to a bundle boundary
    BSA.InstrAlign(bundleSize);

    // We can only split on a bundle boundary, so visit a bundle at a time.
    MachineBasicBlock::iterator FirstI = MBB->begin();
    while (FirstI != MBB->end()) {
        MachineBasicBlock::iterator NextI = FirstI;
        unsigned Bytes = 0;
        
        do {
            Bytes += NextI->getDesc().getSize();
            assert(Bytes <= bundleSize);
            NextI++;
        } while (Bytes < bundleSize && NextI != MBB->end());
        
        // If this visit ends up causing a split, this basic block is over.
        // visitInstrBundle will have set MBB to the proper successor.
        if (visitInstrBundle(MBB, FirstI, NextI))
            return true;
            
        FirstI = NextI;
    }
    
    // No split in this bundle. Next MBB.
    ++MBB;
    return false;
}

bool SVMLateFunctionSplitPass::visitInstrBundle(MachineFunction::iterator &MBB,
    MachineBasicBlock::iterator &FirstI,
    MachineBasicBlock::iterator &EndI)
{
    // If we have nothing to do, return false. If a split occurs, returns
    // true and points 'MBB' at the correct MBB to handle next.
    
    // Visit each instruction, doing early branch rewriting and tracking fn size
    for (MachineBasicBlock::iterator I = FirstI; I != EndI; ++I)
        visitInstr(MBB, I);

    // If any instruction in this bundle put us over budget, split before
    // the whole bundle.
    if (BSA.getByteCount() > TM.getBlockSize()) {
        splitBeforeInstruction(MBB, FirstI);
        return true;
    }
    
    return false;
}

void SVMLateFunctionSplitPass::visitInstr(MachineFunction::iterator &MBB,
    MachineBasicBlock::iterator &I)
{   
    if (SVMInstrInfo::isNearBranchOpcode(I->getOpcode())) {
        /*
         * Unless we can guarantee that a branch target is in the current
         * block, we need to reserve space for a worst-case rewriteBranch().
         */

        // Branch target is always the first opcode
        MachineBasicBlock *TBB = I->getOperand(0).getMBB();
        assert(TBB->getParent() == MBB->getParent());
        unsigned TBBN = TBB->getNumber();

        // In an earlier block? Possibly in a later block?
        if (TBBN < FirstLocalBB || TBBN > (unsigned)MBB->getNumber()) {

            if (SVMInstrInfo::isCondNearBranchOpcode(I->getOpcode())) {
                // Trampoline MBB needed
                BSA.AddInstrSuffix(SVMTargetMachine::getBundleSize());
            }

            // All branches may need a 32-bit constant
            BSA.AddConstant(4, 4);
        }
    }

    BSA.AddInstr(I);
}

void SVMLateFunctionSplitPass::setHasAddressTaken(MachineBasicBlock *MBB) const
{
    // Mark both the MBB and BB as having their address taken, therefore
    // ensuring that we get a label for the MBB.

    MachineFunction *MF = MBB->getParent();
    Function *F = const_cast<Function *>(MF->getFunction());
    BasicBlock *BB = const_cast<BasicBlock *>(MBB->getBasicBlock());
    
    MBB->setHasAddressTaken();
    BlockAddress::get(F, BB);    
}

MachineBasicBlock *SVMLateFunctionSplitPass::createAnonymousMBB(MachineFunction *MF)
{
    /*
     * Create an anonymous MBB that we can use as an address operand.
     * It has a distinct but useless BasicBlock attached to it,
     * and the MBB symbol is aliased to the BB symbol.
     *
     * This is kind of annoying to do while we're constantly renumbering the
     * MBBs. To actually set the alias, we remember the MMI address label
     * symbol and the MBB address, and actually fixup the alias later.
     */

    Function *F = const_cast<Function *>(MF->getFunction());
    BasicBlock *BB = BasicBlock::Create(F->getContext());
    MachineBasicBlock *MBB = MF->CreateMachineBasicBlock(BB);
    setHasAddressTaken(MBB);
    
    // Align the new MBB. On earlier MBBs, this was already set by SVMAlignPass.
    MBB->setAlignment(SVMTargetMachine::getBundleSize());

    AnonMBBFixupList.push_back(std::make_pair(
        MF->getMMI().getAddrLabelSymbol(BB), MBB ));

    return MBB;
}

void SVMLateFunctionSplitPass::fixupAnonymousMBB(MachineFunction *MF)
{
    // Fix up dangling symbols from earlier createAnonymousMBB() calls for
    // this MF. Must be called after the MBB numbers have finished changing.
    
    MCContext &MCtx = MF->getContext();

    for (AnonMBBFixupListTy::iterator I = AnonMBBFixupList.begin();
        I != AnonMBBFixupList.end(); ++I) {

        MCSymbol *Alias = I->first;
        MachineBasicBlock *MBB = I->second;
        
        MBB->getSymbol()->setVariableValue(MCSymbolRefExpr::Create(Alias, MCtx));
    }
    
    AnonMBBFixupList.clear();
}

unsigned SVMLateFunctionSplitPass::getLongBranchCPI(MachineBasicBlock *MBB) const
{
    MachineFunction *MF = MBB->getParent();

    setHasAddressTaken(MBB);

    // Build a constant pool entry for a long branch
    LLVMContext &Ctx = MF->getFunction()->getContext();
    SVMConstantPoolMBB *CPV = SVMConstantPoolMBB::Create(Ctx, MBB, SVMCP::LB);
    return MF->getConstantPool()->getConstantPoolIndex(CPV, 4);
}

void SVMLateFunctionSplitPass::splitBeforeInstruction(
    MachineFunction::iterator &MBB, MachineBasicBlock::iterator &I)
{
    // Split a MBB at a block boundary. Updates 'MBB' to point to the
    // next block that needs visiting.
    
    DebugLoc DL;
    const TargetInstrInfo &TII = *TM.getInstrInfo();
    MachineBasicBlock *prevMBB = MBB;
    MachineFunction *MF = prevMBB->getParent();

    // Point to the next existing MBB, our insertion point for nextMBB.
    ++MBB;

    // Split this MBB in two, by creating a new BB and MBB to insert afterward.
    MachineBasicBlock *nextMBB = createAnonymousMBB(MF);
    MF->insert(MBB, nextMBB);
    MF->RenumberBlocks(prevMBB);

    // Back up, point at nextMBB. This is the next MBB we'll visit.
    // Trampolines get inserted before this point.
    --MBB;

    // Move I and all subsequent instructions to the new MBB
    nextMBB->splice(nextMBB->end(), prevMBB, I, prevMBB->end());

    // Insert a long branch from prevMBB to nextMBB
    BuildMI(prevMBB, DL, TII.get(SVM::LB))
        .addConstantPoolIndex(getLongBranchCPI(nextMBB));

    // Rewrite branches in this block which refer to MBBs in past or future blocks.
    // NB: This may insert additional Trampoline MBBs. We need to renumber again.
    rewriteBranchesInRange(MF, FirstLocalBB, nextMBB->getNumber(), MBB);
    MF->RenumberBlocks(prevMBB);

    // *Last* instruction in the last MBB of a block is the SPLIT.
    // This ensures that the next MBB's label points to the right page.
    --MBB;
    BuildMI(MBB, DL, TII.get(SVM::SPLIT));
    ++MBB;

    // Now we're at the beginning of a block again.
    beginBlock();
    FirstLocalBB = nextMBB->getNumber();
}

void SVMLateFunctionSplitPass::rewriteBranchesInRange(MachineFunction *MF,
    unsigned beginBB, unsigned endBB, MachineFunction::iterator insertPoint)
{
    for (unsigned bb = beginBB; bb != endBB; ++bb) {
        MachineBasicBlock *MBB = MF->getBlockNumbered(bb);
        for (MachineBasicBlock::iterator I = MBB->begin(); I != MBB->end();) {

            if (!SVMInstrInfo::isNearBranchOpcode(I->getOpcode())) {
                ++I;
                continue;
            }
            
            // Branch target is always the first opcode
            MachineBasicBlock *TBB = I->getOperand(0).getMBB();
            assert(TBB->getParent() == MBB->getParent());
            unsigned TBBN = TBB->getNumber();

            if (TBBN < beginBB || TBBN >= endBB)
                rewriteBranch(MBB, I, *TBB, insertPoint);
            else
                ++I;
        }
    }
}

void SVMLateFunctionSplitPass::rewriteBranch(MachineBasicBlock *MBB,
    MachineBasicBlock::iterator &I, MachineBasicBlock &TBB,
    MachineFunction::iterator insertPoint)
{
    // Rewrite the branch at 'I', and advance to the next instruction.
    
    const TargetInstrInfo &TII = *TM.getInstrInfo();
    MachineFunction *MF = MBB->getParent();
    DebugLoc DL = I->getDebugLoc();
    
    if (SVMInstrInfo::isCondNearBranchOpcode(I->getOpcode())) {
        // Rewrite a conditional branch. Since we'd like the condition
        // evaluation to happen in native code, with a svc only if the branch
        // is taken, we redirect the branch to a small trampoline BB, inserted
        // at the beginning of the block, which does an unconditional long
        // branch to the real destination.

        MachineBasicBlock *trMBB = createAnonymousMBB(MF);
        MF->insert(insertPoint, trMBB);
        I->getOperand(0).setMBB(trMBB);
        
        BuildMI(trMBB, DL, TII.get(SVM::LB))
            .addConstantPoolIndex(getLongBranchCPI(&TBB));
            
        ++I;

    } else {
        // Rewrite an unconditional branch. This just replaces the original
        // instruction with a new unconditional branch instruction.

        BuildMI(*MBB, I, DL, TII.get(SVM::LB))
            .addConstantPoolIndex(getLongBranchCPI(&TBB));

        MachineBasicBlock::iterator replacedBranch = I;
        ++I;
        replacedBranch->eraseFromParent();
    }
}
