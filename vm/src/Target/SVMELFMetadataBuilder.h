/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 *
 * ------------------------------------------------------------------------
 *
 * This file was largely based on the ELFObjectWriter code from LLVM 3.0,
 * distributed under the University of Illinois Open Source License.
 * 
 * Copyright (c) 2003-2011 University of Illinois at Urbana-Champaign.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal with
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimers.
 *
 *     * Redistributions in binary form must reproduce the above copyright notice,
 *       this list of conditions and the following disclaimers in the
 *       documentation and/or other materials provided with the distribution.
 *
 *     * Neither the names of the LLVM Team, University of Illinois at
 *       Urbana-Champaign, nor the names of its contributors may be used to
 *       endorse or promote products derived from this Software without specific
 *       prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
 * SOFTWARE.
 */

/*
 * The SVMELFMetadataBuilder is responsible for creating debug-only
 * metadata sections: section names, symbol names, debug symbols.
 *
 * Since this code is debug-only and largely adapted from existing LLVM
 * code, it's kept separate from SVMELFProgramWriter itself.
 */

#ifndef SVM_ELFMETADATABUILDER_H
#define SVM_ELFMETADATABUILDER_H

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCAsmLayout.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCELFSymbolFlags.h"
#include "llvm/Support/ELF.h"


namespace llvm {

    class SVMMemoryLayout;

    class MCELF {
    public:
        static void SetBinding(MCSymbolData &SD, unsigned Binding);
        static unsigned GetBinding(const MCSymbolData &SD);
        static void SetType(MCSymbolData &SD, unsigned Type);
        static unsigned GetType(const MCSymbolData &SD);
        static void SetVisibility(MCSymbolData &SD, unsigned Visibility);
        static unsigned GetVisibility(MCSymbolData &SD);
    };

    class SVMELFMetadataBuilder {
    private:
        typedef DenseMap<const MCSectionELF*, uint32_t> SectionIndexMapTy;

        struct ELFSymbolData {
            MCSymbolData *SymbolData;
            uint64_t StringIndex;
            uint32_t SectionIndex;

            // Support lexicographic sorting.
            bool operator<(const ELFSymbolData &RHS) const {
                if (MCELF::GetType(*SymbolData) == ELF::STT_FILE)
                    return true;
                if (MCELF::GetType(*RHS.SymbolData) == ELF::STT_FILE)
                    return false;
                return SymbolData->getSymbol().getName() <
                    RHS.SymbolData->getSymbol().getName();
            }
        };

        // Symbols
        SmallString<256> StringTable;
        std::vector<ELFSymbolData> LocalSymbolData;

        // Sections
        unsigned ShstrtabIndex;
        unsigned SymbolTableIndex;
        unsigned StringTableIndex;
        DenseMap<const MCSection*, uint32_t> SectionStringTableIndex;

    public:    
        void BuildSections(MCAssembler &Asm, const MCAsmLayout &Layout,
            const SVMMemoryLayout &ML);

        uint32_t getShstrtabIndex() const {
            return ShstrtabIndex;
        }

        uint32_t getStringTableIndex() const {
            return StringTableIndex;
        }

        uint32_t getSectionStringTableIndex(const MCSection *S) {
            return SectionStringTableIndex[S];
        }
        
        uint32_t getLastLocalSymbolIndex() const {
            return LocalSymbolData.size() + 1;
        }
        
    private:
        bool isInSymtab(const MCAssembler &Asm, const MCSymbolData &Data);
        
        void ComputeIndexMap(MCAssembler &Asm, SectionIndexMapTy &SectionIndexMap);
        void ComputeSymbolTable(MCAssembler &Asm, const SectionIndexMapTy &SectionIndexMap);
        
        void WriteSymbolTable(MCDataFragment *SymtabF, const MCAssembler &Asm,
            const MCAsmLayout &Layout, const SectionIndexMapTy &SectionIndexMap,
            const SVMMemoryLayout &ML);

        void WriteSymbolEntry(MCDataFragment *SymtabF, uint32_t name,
            uint8_t info, uint32_t value, uint32_t size, uint8_t other,
            uint32_t shndx);
        void WriteSymbol(MCDataFragment *SymtabF, ELFSymbolData &MSD,
            const MCAssembler &Asm, const MCAsmLayout &Layout,
            const SVMMemoryLayout &ML);
        uint32_t SymbolValue(MCSymbolData &Data, const MCAssembler &Asm,
            const MCAsmLayout &Layout, const SVMMemoryLayout &ML);
    };

}  // end namespace

#endif
