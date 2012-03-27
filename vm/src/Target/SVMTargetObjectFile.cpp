/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMTargetObjectFile.h"
#include "llvm/Support/ELF.h"
#include "llvm/MC/MCContext.h"
using namespace llvm;


void SVMTargetObjectFile::Initialize(MCContext &Ctx, const TargetMachine &TM)
{
    TargetLoweringObjectFileELF::Initialize(Ctx, TM);

    Ctx.getELFSection(".metadata", ELF::SHT_PROGBITS, 0, SectionKind::getMetadata());
    Ctx.getELFSection(".debug_logstr", ELF::SHT_PROGBITS, 0, SectionKind::getMetadata());
}
