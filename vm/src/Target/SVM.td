/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

include "llvm/Target/Target.td"

include "Target/SVMRegisterInfo.td"
include "Target/SVMCallingConv.td"
include "Target/SVMInstrFormats.td"
include "Target/SVMInstrInfo.td"

def SVMInstrInfo : InstrInfo;

def SVMAsmWriter : AsmWriter {
  string AsmWriterClassName  = "InstPrinter";
  bit isMCAsmWriter = 1;
}

def SVM : Target {
    let InstructionSet = SVMInstrInfo;
    let AssemblyWriters = [SVMAsmWriter];
}
