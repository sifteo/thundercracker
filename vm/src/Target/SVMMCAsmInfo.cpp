/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMMCAsmInfo.h"
#include "llvm/ADT/Triple.h"

using namespace llvm;

SVMMCAsmInfo::SVMMCAsmInfo(const Target &T, StringRef TT) {
    Data16bitsDirective = "\t.d16\t";
    Data32bitsDirective = "\t.d32\t";
    Data64bitsDirective = "\r.d64\t";
    ZeroDirective = "\t.zero\t";
    CommentString = "#";
    UsesELFSectionDirectiveForBSS = true;
    PrivateGlobalPrefix = ".L";
    SupportsDebugInformation = true;
}
