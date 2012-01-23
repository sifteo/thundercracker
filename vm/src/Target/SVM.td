/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

include "llvm/Target/Target.td"

include "SVMRegisterInfo.td"
include "SVMCallingConv.td"
include "SVMInstrInfo.td"

def SVMInstrInfo : InstrInfo;

def SVM : Target {
    let InstructionSet = SVMInstrInfo;
}
