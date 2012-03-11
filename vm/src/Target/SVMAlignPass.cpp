/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * This pass aligns all branch targets and all 32-bit instructions to
 * a 32-bit boundary:
 *
 *  1) Basic blocks are always aligned trivially via setAlignment()
 *
 *  2) 32-bit instructions are aligned by inserting NOPs or moving instructions.
 *
 *  3) Non-tail calls are aligned such that the *next* instruction is on
 *     a 32-bit boundary.
 *
 * The instruction alignment done in this manner has a couple benefits:
 *
 *  1) When calculating the final size of a basic block or function,
 *     we don't have to worry about per-instruction alignment.
 *
 *  2) Much like Sparc's DelaySlotFiller pass, we can try to put useful
 *     instructions in before a particular 32-bit instruction, instead of
 *     wasting space with a NOP.
 *
 * This uses a simple algorithm that always makes forward progress. The
 * required 16-bit instruction opening prior to an otherwise-unaligned 32-bit
 * instruction is termed an "align slot". We scan forward (never backward) to
 * find a candidate instruction to move into the align slot. We never cross
 * a funtion call, syscall, or basic block boundary, and we keep track of
 * register dependencies. If no candidate instruction can be found, we give
 * up and insert a no-op.
 */

#include "SVM.h"
#include "SVMTargetMachine.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/codeGen/MachineInstr.h"
#include "llvm/codeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
using namespace llvm;

namespace {

    class SVMAlignPass : public MachineFunctionPass {
        SVMTargetMachine& TM;
        public:
    public:
        static char ID;
        explicit SVMAlignPass(SVMTargetMachine &tm)
            : MachineFunctionPass(ID), TM(tm) {}

        bool runOnMachineFunction(MachineFunction &MF);

        const char *getPassName() const {
            return "SVM alignment pass";
        }
        
    private:
        typedef SmallSet<unsigned, 32> RegSet_t;

        bool runOnMachineBasicBlock(MachineBasicBlock &MBB);

        MachineBasicBlock::iterator findAlignSlotFiller(
            MachineBasicBlock &MBB, MachineBasicBlock::iterator I);

        bool isRegInSet(RegSet_t &RegSet, unsigned Reg);
        void insertRegDefsUses(const MachineInstr &MI,
            RegSet_t &RegDefs, RegSet_t &RegUses);
        bool hasRegHazard(const MachineInstr &MI,
            RegSet_t &RegDefs, RegSet_t &RegUses);
    };
  
    char SVMAlignPass::ID = 0;
}

FunctionPass *llvm::createSVMAlignPass(SVMTargetMachine &TM)
{
    return new SVMAlignPass(TM);
}

bool SVMAlignPass::runOnMachineFunction(MachineFunction &MF)
{
    bool Changed = false;

    for (MachineFunction::iterator I = MF.begin(), E = MF.end(); I != E; ++I)
        if (runOnMachineBasicBlock(*I))
            Changed = true;

    return Changed;
}

bool SVMAlignPass::runOnMachineBasicBlock(MachineBasicBlock &MBB)
{
    const TargetInstrInfo &TII = *TM.getInstrInfo();
    unsigned halfwordCount = 0;
    bool Changed = false;
    
    // All basic blocks must align to a bundle boundary
    if (MBB.getAlignment() != TM.getBundleSize()) {
        assert(MBB.getAlignment() <= TM.getBundleSize());
        MBB.setAlignment(TM.getBundleSize());
        Changed = true;
    }

    // Find and fill all align slots
    for (MachineBasicBlock::iterator I = MBB.begin(); I != MBB.end(); ++I) {
        const MCInstrDesc &Desc = I->getDesc();
        int Size = Desc.getSize();
        assert(Size == 0 || Size == 2 || Size == 4);
        
        if (Size == 4 && (halfwordCount & 1)) {
            // This is an align slot. Fill it! (And count the filler instruction)
            MachineBasicBlock::iterator D = findAlignSlotFiller(MBB, I);
            
            if (D == MBB.end()) {
                BuildMI(MBB, I, I->getDebugLoc(), TII.get(SVM::NOP));
            } else {
                assert(D->getDesc().getSize() == 2);
                MBB.splice(I, &MBB, D);
            }
            
            Changed = true;
            halfwordCount++;
            assert((halfwordCount & 1) == 0);

        } else if ((I->getOpcode() == SVM::CALL || I->getOpcode() == SVM::CALLr)
            && !(halfwordCount & 1)) {
            
            // Non-tail call that *is* aligned. Insert a NOP beforehand, so that
            // the return address will be aligned on a halfword boundary.
            //
            // We also do this for syscalls, not because syscalls themselves
            // require an aligned return address, but because certain syscalls
            // can trigger event dispatch, which *does* require aligned return
            // addresses. One future optimization opportunity is to skip this
            // alignment for syscalls that never need event dispatch.
            //
            // Right now, we already skip this alignment on many common syscalls
            // since we do not align SYS64_CALL instructions, which are used for
            // many common operations, but *not* for the main event disptachers:
            // _SYS_yield(), _SYS_paint(), and _SYS_finish().

            assert(Size == 2);
            BuildMI(MBB, I, I->getDebugLoc(), TII.get(SVM::NOP));
            Changed = true;
            halfwordCount++;
        }

        // Count the original instruction, without align slot filler
        halfwordCount += Size / sizeof(uint16_t);
    }
    
    return Changed;
}

bool SVMAlignPass::isRegInSet(RegSet_t &RegSet, unsigned Reg)
{
    // Register itself is in the set?
    if (RegSet.count(Reg))
        return true;

    // Any of the register's aliases in the set?
    for (const unsigned *Alias = TM.getRegisterInfo()->getAliasSet(Reg);
        *Alias; ++ Alias)
        if (RegSet.count(*Alias))
            return true;

    return false;
}

void SVMAlignPass::insertRegDefsUses(const MachineInstr &MI,
    RegSet_t &RegDefs, RegSet_t &RegUses)
{
    for (unsigned i = 0, e = MI.getNumOperands(); i != e; ++i) {
        const MachineOperand &MO = MI.getOperand(i);
        
        if (MO.isReg() && MO.getReg()) {
            if (MO.isDef())
                RegDefs.insert(MO.getReg());
            if (MO.isUse())
                RegUses.insert(MO.getReg());
        }
    }
}

bool SVMAlignPass::hasRegHazard(const MachineInstr &MI,
    RegSet_t &RegDefs, RegSet_t &RegUses)
{
    for (unsigned i = 0, e = MI.getNumOperands(); i != e; ++i) {
        const MachineOperand &MO = MI.getOperand(i);

        if (MO.isReg() && MO.getReg()) {

            // Using a register that was redefined
            if (MO.isUse() && isRegInSet(RegDefs, MO.getReg()))
                return true;

            // Redefining a register that is used
            if (MO.isDef() && isRegInSet(RegUses, MO.getReg()))
                return true;

            // Defining a register that will be overwritten
            if (MO.isDef() && isRegInSet(RegDefs, MO.getReg()))
                return true;            
        }
    }

    return false;
}

MachineBasicBlock::iterator SVMAlignPass::findAlignSlotFiller(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator I)
{
    /*
     * Find a candidate filler instruction for an align slot at 'I',
     * in basic block 'MBB'. If nothing can be found, returns MBB.end().
     */

    RegSet_t RegDefs, RegUses;

    for (;I != MBB.end(); ++I) {
        const MCInstrDesc &Desc = I->getDesc();

        // Instructions that terminate the search
        if (I->hasUnmodeledSideEffects() || I->isInlineAsm() ||
            I->isLabel() || Desc.isCall() || Desc.isBranch() || Desc.isReturn())
            break;

        // Can this instruction be a filler?
        if (!I->isDebugValue() && Desc.getSize() == 2 &&
            !Desc.mayLoad() && !Desc.mayStore() &&
            !hasRegHazard(*I, RegDefs, RegUses))
            return I;

        insertRegDefsUses(*I, RegDefs, RegUses);
    }

    return MBB.end();
}
