/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo VM (SVM) Target for LLVM
 *
 * Micah Elizabeth Scott <micah@misc.name>
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
{ "fixup_relcpi",            0,      8,    MCFixupKindInfo::FKF_IsPCRel }, \
{ "fixup_abscpi",            0,      7,    0 }, \
{ "fixup_fnstack",           0,      0,    0 }, \

fixup_bcc = FirstTargetFixupKind,
fixup_b,
fixup_call,
fixup_relcpi,
fixup_abscpi,
fixup_fnstack,

LastTargetFixupKind,
NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
};
}
}

#endif
