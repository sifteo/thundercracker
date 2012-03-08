/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */
 
/*
 * Our AsmPrinter has some responsibilities that are peculiar to SVM.
 * This is where we handle actually placing code and data into flash blocks.
 * We rely on annotations and fixups from previous passes, such as
 * SVMLateFunctionSplitPass, but this is where we actually:
 *
 *  1. Replace 'split' pseudo-instructions with actual block alignment.
 *  2. Split LLVM constpools into per-block constant pools.
 *
 * There are some substantial similarities between this and the ARM target's
 * ARMConstantIslandPass, but we divide the work between LateFunctionSplitPass
 * and the AsmPrinter so that we can do our lower-level and non-per-function
 * tasks here. For example, two small functions that get output into the
 * same flash block must be able to share a constant pool.
 *
 * It turns out that this all makes our job substantially simpler than
 * ARMConstantIslandPass, since we already know exactly where the splits go,
 * and we have no BB reshuffing or branch rewriting to take care of at this
 * point.
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

    // Rewrite CPI operands
    for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
        const MachineOperand &MO = MI->getOperand(i);
        if (MO.isCPI()) {
            emitConstRefComment(MO);
            rewriteConstForCurrentBlock(MO, MCI.getOperand(i));
        }
    }

    // Special pseudo-operations
    switch (MI->getOpcode()) {
        
    case SVM::SPLIT:
        emitBlockSplit(MI);
        break;
    
    default:
        break;
    }

    // A small kludge for reporting MBB alignment to BSA...
    const MachineBasicBlock *MBB = MI->getParent();
    if (MI == MBB->begin())
        BSA.InstrAlign(MBB->getAlignment());

    BSA.AddInstr(MI);
    emitBlockOffsetComment();
    OutStreamer.EmitInstruction(MCI);
}

void SVMAsmPrinter::EmitFunctionEntryLabel()
{
    OutStreamer.ForceCodeRegion();

    // XXX: For now, all functions start in new blocks
    emitBlockBegin();

    CurrentFnSplitOrdinal = 0;
    emitFunctionLabelImpl(CurrentFnSym);
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

void SVMAsmPrinter::emitFunctionLabelImpl(MCSymbol *Sym)
{
    // Emit a label, annotated for function-ness.

    OutStreamer.EmitAssemblerFlag(MCAF_Code16);
    OutStreamer.EmitThumbFunc(Sym);
    OutStreamer.EmitLabel(Sym); 
    OutStreamer.EmitSymbolAttribute(Sym, MCSA_Global);
    OutStreamer.EmitSymbolAttribute(Sym, MCSA_ELF_TypeFunction);    
}

void SVMAsmPrinter::emitBlockBegin()
{
    BlockConstPool.clear();
    BSA.clear();

    OutStreamer.EmitValueToAlignment(
        SVMTargetMachine::getBlockSize(),
        SVMTargetMachine::getPaddingByte());
}

void SVMAsmPrinter::emitBlockEnd()
{
    OutStreamer.EmitValueToAlignment(sizeof(uint32_t),
        SVMTargetMachine::getPaddingByte());    

    emitBlockConstPool();
}

void SVMAsmPrinter::emitBlockSplit(const MachineInstr *MI)
{
    emitBlockEnd();
    emitBlockBegin();

    // Create a global label for the split-off portion of this function.
    // These labels are never used by us, they're just for debugging.
    assert(CurrentFnSym);
    Twine Name(CurrentFnSym->getName() + Twine(".")
        + Twine(++CurrentFnSplitOrdinal));
    emitFunctionLabelImpl(OutContext.GetOrCreateSymbol(Name));
}

void SVMAsmPrinter::EmitMachineConstantPoolValue(MachineConstantPoolValue *MCPV)
{
    int Size = TM.getTargetData()->getTypeAllocSize(MCPV->getType());
    const SVMConstantPoolValue *SCPV = static_cast<SVMConstantPoolValue*>(MCPV);
    MCSymbol *MCSym;

    if (SCPV->isMachineBasicBlock()) {
        const MachineBasicBlock *MBB = cast<SVMConstantPoolMBB>(SCPV)->getMBB();
        MCSym = MBB->getSymbol();
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
        if (!MCDecoratedSym->isVariable())
            MCDecoratedSym->setVariableValue(
                MCSymbolRefExpr::Create(MCSym, OutContext));
        break;
    }
    
    const MCExpr *Expr = MCSymbolRefExpr::Create(MCDecoratedSym, OutContext);
    OutStreamer.EmitValue(Expr, Size);  
}

void SVMAsmPrinter::emitConstRefComment(const MachineOperand &MO)
{
    assert(MO.isCPI());
    raw_ostream &OS = OutStreamer.GetCommentOS();

    const std::vector<MachineConstantPoolEntry> &Constants =
        MF->getConstantPool()->getConstants();

    assert((unsigned)MO.getIndex() < Constants.size());
    const MachineConstantPoolEntry &Entry = Constants[MO.getIndex()];

    OS << "pool: ";

    if (Entry.isMachineConstantPoolEntry())
        Entry.Val.MachineCPVal->print(OS);
    else if (Entry.Val.ConstVal->hasName())
        OS << Entry.Val.ConstVal->getName();
    else
        OS << *(Value*)Entry.Val.ConstVal;            
    OS << "\n";
}

void SVMAsmPrinter::emitBlockOffsetComment()
{
    raw_ostream &OS = OutStreamer.GetCommentOS();

    unsigned byteCount = BSA.getByteCount();
    if (byteCount > SVMTargetMachine::getBlockSize())
        report_fatal_error("Block overflow (" + Twine(byteCount) + " bytes) "
            "in function " + Twine(CurrentFnSym->getName()) + "." +
            Twine(CurrentFnSplitOrdinal));

    OS << "BSA: ";
    BSA.describe(OS);
    OS << "\n";
}

void SVMAsmPrinter::rewriteConstForCurrentBlock(const MachineOperand &MO, MCOperand &MCO)
{
    /*
     * Rewrite a constant pool expression such that instead of referring to LLVM's
     * per-function constant pool (which we don't actually emit) it refers to our
     * own per-block constant pool.
     *
     * On entry, MO is the original CPI operand, and MCO has been lowered as a
     * MCSymbolRefExpr pointing to LLVM's CPI label. We overwrite MCO with a
     * new MCSymbolRefExpr referring to the entry that we'll emit later in
     * emitBlockConstPool().
     *
     * For convenience, we internally unique the per-block CP entries according
     * to this original MCSymbol, since LLVM has already gone to the trouble to
     * unique these.
     */

    assert(MO.isCPI());
    assert(MCO.isExpr());

    const MachineConstantPool *MCP = MF->getConstantPool();
    const std::vector<MachineConstantPoolEntry> &CP = MCP->getConstants();

    const MCSymbolRefExpr *SRE = dyn_cast<MCSymbolRefExpr>(MCO.getExpr());
    assert(SRE);
    const MCSymbol *Key = &SRE->getSymbol();

    BlockConstPoolTy::iterator I = BlockConstPool.find(Key);
    if (I == BlockConstPool.end()) {
        // Add a new constant to this block's pool
        CPEInfo Info(OutContext.CreateTempSymbol(), &CP[MO.getIndex()]);
        I = BlockConstPool.insert(std::make_pair(Key, Info)).first;
    }

    MCO.setExpr(MCSymbolRefExpr::Create(I->second.Symbol, OutContext));
}

void SVMAsmPrinter::emitBlockConstPool()
{
    /*
     * Emit all of the CPEs created by rewriteConstForCurrentBlock.
     * This is a SVM-specific per-block constant pool, distinct from
     * LLVM's usual per-function constant pools.
     */

    for (BlockConstPoolTy::iterator I = BlockConstPool.begin(),
        E = BlockConstPool.end(); I != E; ++I) {
        CPEInfo &Info = I->second;

        OutStreamer.EmitLabel(Info.Symbol);

        if (Info.MCPE->isMachineConstantPoolEntry())
            EmitMachineConstantPoolValue(Info.MCPE->Val.MachineCPVal);
        else
            EmitGlobalConstant(Info.MCPE->Val.ConstVal);
    }
}
