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

#ifndef SVM_MCASMBACKEND_H
#define SVM_MCASMBACKEND_H

#include "SVM.h"
#include "SVMMCTargetDesc.h"
#include "SVMFixupKinds.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCDirectives.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ELF.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Triple.h"

namespace llvm {

class SVMAsmBackend : public MCAsmBackend {
public:    
    SVMAsmBackend(const Target &T, Triple::OSType O)
        : MCAsmBackend(), OSType(O) {}

    Triple::OSType OSType;

    MCObjectWriter *createObjectWriter(raw_ostream &OS) const;
    unsigned getNumFixupKinds() const;
    
    const MCFixupKindInfo &getFixupKindInfo(MCFixupKind Kind) const;
    static const MCFixupKindInfo &getStaticFixupKindInfo(MCFixupKind Kind);

    void ApplyFixup(const MCFixup &Fixup, char *Data, unsigned DataSize,
        uint64_t Value) const;
    static void ApplyStaticFixup(MCFixupKind Kind, char *Data, int32_t Value);

    bool MayNeedRelaxation(const MCInst &Inst) const;
    void RelaxInstruction(const MCInst &Inst, MCInst &Res) const;

    bool WriteNopData(uint64_t Count, MCObjectWriter *OW) const;
};

}  // end namespace

#endif
