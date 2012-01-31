/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_FIXUPKINDS_H
#define SVM_FIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace SVM {
enum Fixups {

// Name                      Offset  Size  Flags
#define DEFINE_FIXUP_KIND_INFO \
{ "fixup_bcc",               0,      8,    MCFixupKindInfo::FKF_IsPCRel }, \
{ "fixup_b",                 0,      11,   MCFixupKindInfo::FKF_IsPCRel }, \
{ "fixup_call",              0,      8,    0 }, \
{ "fixup_cpi",               0,      8,    MCFixupKindInfo::FKF_IsPCRel }, \

fixup_bcc = FirstTargetFixupKind,
fixup_b,
fixup_call,
fixup_cpi,

LastTargetFixupKind,
NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
};
}
}

#endif
