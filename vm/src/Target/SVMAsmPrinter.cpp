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
#include "llvm/CodeGen/MachineConstantPool.h"
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

    const TargetInstrInfo &TII = *TM.getInstrInfo();
    const MCInstrDesc &Desc = TII.get(MCI.getOpcode());
    int Size = Desc.getSize();

    // Word-align 32-bit instructions
    if (Size > 2)
        OutStreamer.EmitCodeAlignment(Size);
    
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

void SVMAsmPrinter::EmitConstantPool()
{
    /*
     * Do nothing. We disable the default implementation and do this
     * ourselves, since constants need to be placed at the end of each
     * memory block.
     */
}

void SVMAsmPrinter::EmitFunctionBodyEnd()
{
    /* XXX: Kludge to emit constants at the end of each function */

    const MachineConstantPool *MCP = MF->getConstantPool();
    const std::vector<MachineConstantPoolEntry> &CP = MCP->getConstants();

    // Ensure we're at a 32-bit boundary
    EmitAlignment(2);
    
    for (unsigned i = 0, end = CP.size(); i != end; i++) {
       MachineConstantPoolEntry CPE = CP[i];

       OutStreamer.EmitLabel(GetCPISymbol(i));

       if (CPE.isMachineConstantPoolEntry())
           EmitMachineConstantPoolValue(CPE.Val.MachineCPVal);
       else
           EmitGlobalConstant(CPE.Val.ConstVal);
    }
}

