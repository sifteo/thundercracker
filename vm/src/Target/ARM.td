//===- ARM.td - Describe the ARM Target Machine ------------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Target-independent interfaces which we are implementing
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

include "ARMSchedule.td"

class ProcNoItin<string Name, list<SubtargetFeature> Features>
 : Processor<Name, GenericItineraries, Features>;

def : ProcNoItin<"generic",         []>;

//===----------------------------------------------------------------------===//
// Register File Description
//===----------------------------------------------------------------------===//

include "ARMRegisterInfo.td"

include "ARMCallingConv.td"

//===----------------------------------------------------------------------===//
// Instruction Descriptions
//===----------------------------------------------------------------------===//

include "ARMInstrInfo.td"

def ARMInstrInfo : InstrInfo;


//===----------------------------------------------------------------------===//
// Assembly printer
//===----------------------------------------------------------------------===//
// ARM Uses the MC printer for asm output, so make sure the TableGen
// AsmWriter bits get associated with the correct class.
def ARMAsmWriter : AsmWriter {
  string AsmWriterClassName  = "InstPrinter";
  bit isMCAsmWriter = 1;
}

//===----------------------------------------------------------------------===//
// Declare the target which we are implementing
//===----------------------------------------------------------------------===//

def ARM : Target {
  // Pull in Instruction Info:
  let InstructionSet = ARMInstrInfo;

  let AssemblyWriters = [ARMAsmWriter];
}
