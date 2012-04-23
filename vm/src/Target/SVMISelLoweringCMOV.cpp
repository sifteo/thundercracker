/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 *
 * ------------------------------------------------------------------------
 *
 * This file was largely based on the Mips target from LLVM 3.0,
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

#include "SVMISelLowering.h"
#include "SVMTargetMachine.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/ErrorHandling.h"
using namespace llvm;


MachineBasicBlock *SVMTargetLowering::ExpandCMOV(MachineInstr *MI,
    MachineBasicBlock *BB) const
{
    // (CMOV GPReg:$Rd, GPReg:$T, GPReg:$F, CCop:$cc, CPSRReg:$c)

    const TargetInstrInfo *TII = getTargetMachine().getInstrInfo();
    DebugLoc dl = MI->getDebugLoc();

    // To "insert" a CMOV instruction, we actually have to insert the
    // diamond control-flow pattern.  The incoming instruction knows the
    // destination vreg to set, the condition code register to branch on, the
    // true/false values to select between, and a branch opcode to use.

    const BasicBlock *LLVM_BB = BB->getBasicBlock();
    MachineFunction::iterator It = BB;
    ++It;

    MachineBasicBlock *thisMBB  = BB;
    MachineFunction *F = BB->getParent();
    MachineBasicBlock *copy0MBB = F->CreateMachineBasicBlock(LLVM_BB);
    MachineBasicBlock *sinkMBB  = F->CreateMachineBasicBlock(LLVM_BB);
    F->insert(It, copy0MBB);
    F->insert(It, sinkMBB);

    // Transfer the remainder of BB and its successor edges to sinkMBB.
    sinkMBB->splice(sinkMBB->begin(), BB,
                    llvm::next(MachineBasicBlock::iterator(MI)),
                    BB->end());
    sinkMBB->transferSuccessorsAndUpdatePHIs(BB);

    // Next, add the true and fallthrough blocks as its successors.
    BB->addSuccessor(copy0MBB);
    BB->addSuccessor(sinkMBB);

    // Emit the conditional branch op
    BuildMI(BB, dl, TII->get(SVM::Bcc))
        .addMBB(sinkMBB)                // Target
        .addOperand(MI->getOperand(3))  // CCop
        .addOperand(MI->getOperand(4)); // CPSRReg

    //  copy0MBB:
    //   %FalseValue = ...
    //   # fallthrough to sinkMBB
    BB = copy0MBB;

    // Update machine-CFG edges
    BB->addSuccessor(sinkMBB);

    //  sinkMBB:
    //   %Result = phi [ %TrueValue, thisMBB ], [ %FalseValue, copy0MBB ]
    //  ...
    BB = sinkMBB;
    BuildMI(*BB, BB->begin(), dl,
            TII->get(SVM::PHI), MI->getOperand(0).getReg())
      .addReg(MI->getOperand(1).getReg()).addMBB(thisMBB)
      .addReg(MI->getOperand(2).getReg()).addMBB(copy0MBB);

    MI->eraseFromParent();   // The pseudo instruction is gone now.
    return BB;
}
