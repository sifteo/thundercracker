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
#include "SVMConstantPoolValue.h"
#include "SVMSymbolDecoration.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCExpr.h"
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

    if (OutStreamer.isVerboseAsm()) {
        for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i)
            emitOperandComment(MI, MI->getOperand(i));
    }
    
    switch (MI->getOpcode()) {
        
    case SVM::SPLIT:
        // XXX: Need to fix constpool split
        //emitBlockEnd();
        emitBlockBegin();
        break;
    
    default:
        break;
    }

    OutStreamer.EmitInstruction(MCI);
}

void SVMAsmPrinter::emitOperandComment(const MachineInstr *MI, const MachineOperand &OP)
{
    if (OP.isCPI()) {
        raw_ostream &OS = OutStreamer.GetCommentOS();

        const std::vector<MachineConstantPoolEntry> &Constants =
            MI->getParent()->getParent()->getConstantPool()->getConstants();

        assert((unsigned)OP.getIndex() < Constants.size());
        const MachineConstantPoolEntry &Entry = Constants[OP.getIndex()];

        OS << "pool: ";

        if (Entry.isMachineConstantPoolEntry())
            Entry.Val.MachineCPVal->print(OS);
        else if (Entry.Val.ConstVal->hasName())
            OS << Entry.Val.ConstVal->getName();
        else
            OS << *(Value*)Entry.Val.ConstVal;            
        OS << "\n";
    }
}

void SVMAsmPrinter::EmitFunctionEntryLabel()
{
    OutStreamer.ForceCodeRegion();

    // XXX: For now, all functions start in new blocks
    emitBlockBegin();

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
    // XXX: For now, all functions end a block
    emitBlockEnd();
}

void SVMAsmPrinter::emitBlockBegin()
{
    OutStreamer.EmitValueToAlignment(
        SVMTargetMachine::getBlockSize(),
        SVMTargetMachine::getPaddingByte());    
}

void SVMAsmPrinter::emitBlockEnd()
{
    const MachineConstantPool *MCP = MF->getConstantPool();
    const std::vector<MachineConstantPoolEntry> &CP = MCP->getConstants();

    OutStreamer.EmitCodeAlignment(sizeof(uint32_t));
    
    for (unsigned i = 0, end = CP.size(); i != end; i++) {
       MachineConstantPoolEntry CPE = CP[i];

       OutStreamer.EmitLabel(GetCPISymbol(i));

       if (CPE.isMachineConstantPoolEntry())
           EmitMachineConstantPoolValue(CPE.Val.MachineCPVal);
       else
           EmitGlobalConstant(CPE.Val.ConstVal);
    }
}

void SVMAsmPrinter::EmitMachineConstantPoolValue(MachineConstantPoolValue *MCPV)
{
    int Size = TM.getTargetData()->getTypeAllocSize(MCPV->getType());
    const SVMConstantPoolValue *SCPV = static_cast<SVMConstantPoolValue*>(MCPV);
    MCSymbol *MCSym;

    if (SCPV->isMachineBasicBlock()) {
        const MachineBasicBlock *MBB = cast<SVMConstantPoolMBB>(SCPV)->getMBB();
        MCSym = GetBlockAddressSymbol(MBB->getBasicBlock());
        MCSym->setUsed(true);
    } else {
        assert(false && "Unrecognized SVMConstantPoolValue type");
    }

    MCSymbol *MCDecoratedSym;
    switch (SCPV->getModifier()) {
    
    default:
        assert(false && "Unrecognized SVMConstantPoolValue modifier");
    case SVMCP::no_modifier:
        MCDecoratedSym = MCSym;
        break;
    
    case SVMCP::LB:
        // Long branch decoration
        MCDecoratedSym = OutContext.GetOrCreateSymbol(
            SVMDecorations::LB + MCSym->getName());
        MCDecoratedSym->setVariableValue(
            MCSymbolRefExpr::Create(MCSym, OutContext));
        break;
    }
    
    const MCExpr *Expr = MCSymbolRefExpr::Create(MCDecoratedSym, OutContext);
    OutStreamer.EmitValue(Expr, Size);  
}

