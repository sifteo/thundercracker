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
 
/*
 * The SVMELFProgramWriter class is analogous to ELFObjectWriter, but
 * this class writes fully loadable programs, instead of relocatable objects.
 *
 * Luckily, our job is much easier since we don't need to worry about
 * relocations at all. All symbols are written with a fully resolved address.
 *
 * This class is responsible for assigning absolute addresses to all symbols,
 * according to the layout of segments in RAM and Flash.
 */

#ifndef SVM_ELFPROGRAMWRITER_H
#define SVM_ELFPROGRAMWRITER_H

#include "SVM.h"
#include "SVMMCTargetDesc.h"
#include "SVMTargetMachine.h"
#include "SVMMemoryLayout.h"
#include "SVMELFMetadataBuilder.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCValue.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAsmLayout.h"
#include "llvm/Support/ELF.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include <map>
#include <vector>


namespace llvm {

    class SVMELFProgramWriter : public MCObjectWriter {
    public:    
        SVMELFProgramWriter(raw_ostream &OS);

        bool IsSymbolRefDifferenceFullyResolvedImpl(const MCAssembler &Asm,
            const MCSymbolData &DataA, const MCFragment &FB,
            bool InSet, bool IsPCRel) const
        {
            // All symbols are fully resolved
            return true;
        }
    
        void ExecutePostLayoutBinding(MCAssembler &Asm, const MCAsmLayout &Layout)
        {
            // Nothing to do
        }
    
        void RecordRelocation(const MCAssembler &Asm, const MCAsmLayout &Layout,
            const MCFragment *Fragment, const MCFixup &Fixup, MCValue Target,
            uint64_t &FixedValue);

        void WriteObject(MCAssembler &Asm, const MCAsmLayout &Layout);

    private:
        SVMMemoryLayout ML;
        SVMELFMetadataBuilder EMB;
        uint32_t SHOffset;

        void writePadding(unsigned N);
        void padToOffset(uint32_t O);
        void padToAlignment(uint32_t A);

        void writeELFHeader(const MCAssembler &Asm, const MCAsmLayout &Layout);
        void writeProgramHeader(SVMProgramSection S);
        void writeSectionHeader(const MCAsmLayout &Layout, const MCSectionData *SD);  
        void writeDebugMessage();

        void rwCompress(MCAssembler &Asm, const MCAsmLayout &Layout, SVMMemoryLayout &ML);
    };

}  // end namespace

#endif
