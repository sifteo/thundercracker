/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVM.h"
#include "SVMInstrInfo.h"
#include "SVMTargetMachine.h"
#include "SVMMCTargetDesc.h"
#include "SVMMCInstLower.h"
#include "SVMAsmPrinter.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

extern "C" void LLVMInitializeSVMAsmPrinter() { 
    RegisterAsmPrinter<SVMAsmPrinter> X(TheSVMTarget);
}

void SVMAsmPrinter::EmitInstruction(const MachineInstr *MI)
{
    SVMMCInstLower MCInstLowering(Mang, *MF, *this);
    MCInst MCI;
    MCInstLowering.Lower(MI, MCI);
    OutStreamer.EmitInstruction(MCI);
}

void SVMAsmPrinter::EmitFunctionEntryLabel()
{
    OutStreamer.ForceCodeRegion();

    // Always flag this as a thumb function
    OutStreamer.EmitAssemblerFlag(MCAF_Code16);
    OutStreamer.EmitThumbFunc(CurrentFnSym);

    OutStreamer.EmitLabel(CurrentFnSym);
}
