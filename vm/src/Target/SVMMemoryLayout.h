/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
        SPS_META = 0,
        SPS_RO,
        SPS_RW,
        SPS_BSS,
        SPS_DEBUG,
        SPS_NUM_SECTIONS,       // Total number of section types
        SPS_END,                // End of all sections
    };

    // SVM ELF constants
    namespace SVMELF {

        // Program header types
        enum PT {
            PT_METADATA = 0x7000f001,
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
            const MCAsmLayout &Layout, MCValue Value) const;

        uint32_t getSectionDiskSize(enum SVMProgramSection s) const;
        uint32_t getSectionDiskOffset(enum SVMProgramSection s) const;
        uint32_t getSectionDiskEnd(enum SVMProgramSection s) const;

        uint32_t getSectionMemSize(enum SVMProgramSection s) const;
        uint32_t getSectionMemAddress(enum SVMProgramSection s) const;

        uint32_t getSectionDiskOffset(const MCSectionData *SD) const;
        uint32_t getSectionMemAddress(const MCSectionData *SD) const;

        SVMProgramSection getSectionKind(const MCSectionData *SD) const;

    private:
        uint32_t spsSize[SPS_NUM_SECTIONS];
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

        LateFixupList_t LateFixupList;
        FNStackMap_t FNStackMap;
        SectionOffsetMap_t SectionOffsetMap;
    };

}  // end namespace

#endif
