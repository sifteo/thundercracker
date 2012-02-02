/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SVM_MCEXPR_H
#define _SVM_MCEXPR_H

#include "SVMInstrInfo.h"
#include "llvm/MC/MCExpr.h"

namespace llvm {

class SVMMCExpr : public MCTargetExpr {
private:
    SVMTOF::TFlags Flags;
    const MCExpr *Expr;

    explicit SVMMCExpr(SVMTOF::TFlags Flags, const MCExpr *Expr)
        : Flags(Flags), Expr(Expr) {}

public:
    static const SVMMCExpr *Create(
        SVMTOF::TFlags Flags, const MCExpr *Expr, MCContext &Ctx);

    SVMTOF::TFlags getFlags() const { return Flags; }
    const MCExpr *getSubExpr() const { return Expr; }

    void PrintImpl(raw_ostream &OS) const;
    bool EvaluateAsRelocatableImpl(MCValue &Res, const MCAsmLayout *Layout) const;
    void AddValueSymbols(MCAssembler *) const;

    const MCSection *FindAssociatedSection() const {
        return getSubExpr()->FindAssociatedSection();
    }

    static bool classof(const MCExpr *E) {
        return E->getKind() == MCExpr::Target;
    }
  
    static bool classof(const SVMMCExpr *) { return true; }
};

} // end namespace llvm

#endif
