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
 * This is a helper object used by SVMELFProgramWriter to handle all tasks
 * necessary to assign absolute addresses to symbols, functions, and sections.
 *
 * We lay out sections in RAM as well as on disk, apply late fixups, and
 * handle symbol resolution.
 */

#ifndef SVM_MEMORYLAYOUT_H
#define SVM_MEMORYLAYOUT_H

#include "SVM.h"
#include "SVMMCTargetDesc.h"
#include "SVMTargetMachine.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCValue.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAsmLayout.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/ELF.h"
#include <map>
#include <vector>

namespace llvm {

    struct SVMSymbolInfo {
        uint32_t Value;
        enum SymbolKind {
            NONE = 0,
            LOCAL,
            SYS,
            CALL,
            LB,
        } Kind;
        
        SVMSymbolInfo() : Value(0), Kind(NONE) {}
    };

    // Section numbers for the program header
    enum SVMProgramSection {
        // Non-debug sections
        SPS_META = 0,
        SPS_RO,
        SPS_RW_Z,               // Compressed RWDATA section
        SPS_BSS,

        // Debug sections
        SPS_DEBUG,              // Default and first debug section class
        SPS_RW_PLAIN,           // Plaintext debug-only RWDATA section

        SPS_NUM_SECTIONS,       // Total number of section types
        SPS_END,                // End of all sections
    };

    // SVM ELF constants
    namespace SVMELF {

        // Program header types
        enum PT {
            PT_METADATA = 0x7000f001,
            PT_LOAD_FASTLZ = 0x7000f002,
        };

        // Program header layout
        enum PH {
            PHNum = SPS_DEBUG,
            PHSize = PHNum * sizeof(ELF::Elf32_Phdr),
            PHOffset = sizeof(ELF::Elf32_Ehdr),
            HdrSize = PHOffset + PHSize,
        };
    };

    class SVMMemoryLayout {
    public:

        void AllocateSections(MCAssembler &Asm, const MCAsmLayout &Layout);
        void RecordRelocation(const MCAssembler &Asm,
            const MCAsmLayout &Layout, const MCFragment *Fragment,
            const MCFixup &Fixup, MCValue Target, uint64_t &FixedValue);
        void ApplyLateFixups(const MCAssembler &Asm, const MCAsmLayout &Layout);

        SVMSymbolInfo getSymbol(const MCAssembler &Asm,
            const MCAsmLayout &Layout, const MCSymbol *S,
            bool useCodeAddresses = true) const;

        uint32_t getEntryAddress(const MCAssembler &Asm,
            const MCAsmLayout &Layout) const;
        SVMSymbolInfo getSymbol(const MCAssembler &Asm,
            const MCAsmLayout &Layout, MCValue Value,
            bool useCodeAddresses = true) const;

        uint32_t getSectionDiskSize(enum SVMProgramSection s) const;
        uint32_t getSectionDiskOffset(enum SVMProgramSection s) const;
        uint32_t getSectionDiskEnd(enum SVMProgramSection s) const;

        uint32_t getSectionMemSize(enum SVMProgramSection s) const;
        uint32_t getSectionMemAddress(enum SVMProgramSection s) const;

        uint32_t getSectionDiskOffset(const MCSectionData *SD) const;
        uint32_t getSectionMemAddress(const MCSectionData *SD) const;
        uint32_t getSectionMemSize(const MCSectionData *SD, const MCAsmLayout &Layout) const;
        void setSectionMemSize(const MCSectionData *SD, uint32_t size);

        SVMProgramSection getSectionKind(const MCSectionData *SD) const;
        void setSectionKind(const MCSectionData *SD, SVMProgramSection kind);

    private:
        uint32_t spsMemSize[SPS_NUM_SECTIONS];
        uint32_t spsDiskSize[SPS_NUM_SECTIONS];
        uint32_t bssAlign;

        struct SVMLateFixup {
            MCFragment *Fragment;
            uint32_t Offset;
            MCFixupKind Kind;
            MCValue Target;

            SVMLateFixup(MCFragment *Fragment, const MCFixup &Fixup, MCValue Target)
                : Fragment(Fragment), Offset(Fixup.getOffset()),
                  Kind(Fixup.getKind()), Target(Target) {}
        };

        typedef std::vector<SVMLateFixup> LateFixupList_t;
        typedef std::map<const MCSectionData*, uint32_t> SectionOffsetMap_t;
        typedef std::map<std::pair<const MCSectionData *, uint32_t>, int> FNStackMap_t;
        typedef std::map<const MCSectionData*, SVMProgramSection> SectionKindOverrides_t;
        typedef std::map<const MCSectionData*, int32_t> SectionMemSizeOverrides_t;

        LateFixupList_t LateFixupList;
        FNStackMap_t FNStackMap;
        SectionOffsetMap_t SectionOffsetMap;
        SectionKindOverrides_t SectionKindOverrides;
        SectionMemSizeOverrides_t SectionMemSizeOverrides;
    };

}  // end namespace

#endif
