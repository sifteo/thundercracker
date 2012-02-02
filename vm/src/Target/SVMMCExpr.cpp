/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMMCExpr.h"
#include "llvm/MC/MCAssembler.h"
using namespace llvm;

const SVMMCExpr *SVMMCExpr::Create(SVMTOF::TFlags Flags,
    const MCExpr *Expr, MCContext &Ctx)
{
    return new (Ctx) SVMMCExpr(Flags, Expr);
}

void SVMMCExpr::PrintImpl(raw_ostream &OS) const
{
    Expr->print(OS);
}

bool SVMMCExpr::EvaluateAsRelocatableImpl(MCValue &Res,
    const MCAsmLayout *Layout) const
{
    return false;
}

void SVMMCExpr::AddValueSymbols(MCAssembler *Asm) const
{
    assert(Expr->getKind() == MCExpr::SymbolRef);
    Asm->getOrCreateSymbolData(cast<MCSymbolRefExpr>(Expr)->getSymbol());
}
