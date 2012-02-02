/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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

    /*
     * Magic symbol prefixes.
     *
     * These are used as an ad-hoc way of passing annotations to the
     * "linker" in SVMELFProgramWriter. We have some very nonstandard
     * features in the SVM architecture: syscalls that look like functions,
     * function calls and tail calls that encode the instruction op into
     * the target pointer, etc.
     *
     * Since LLVM has no native way to represent these types cleanly, as
     * far as I'm aware, this approach lets us pass custom info to our
     * linker without patching the LLVM core.
     */
    
    namespace SVMPrefix {
        // Prefixes that are intended for use in source code
        static const char SYS[] = "_SYS_";
        
        // For internal use
        static const char CALL[] = "_call$";
        static const char TCALL[] = "_tcall$";
    }
    
    typedef std::vector<const MCSectionData*> SectionDataList;    
    typedef std::map<const MCSectionData*, uint32_t> SectionBaseAddrMap;

    struct SVMSymbolInfo {
        uint32_t Value;
        enum SymbolKind {
            NONE = 0,
            LOCAL,
            SYS,
            CALL,
        } Kind;
        
        SVMSymbolInfo() : Value(0), Kind(NONE) {}
    };

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
        SectionBaseAddrMap SectionAddr;
        uint32_t ramTopAddr;
        uint32_t flashTopAddr;

        uint32_t getEntryAddress(const MCAssembler &Asm,
            const MCAsmLayout &Layout);
        SVMSymbolInfo getSymbol(const MCAssembler &Asm,
            const MCAsmLayout &Layout, const MCSymbol *S);
        SVMSymbolInfo getSymbol(const MCAssembler &Asm,
            const MCAsmLayout &Layout, MCValue Value);
    
        void collectProgramSections(const MCAssembler &Asm,
            const MCAsmLayout &Layout, SectionDataList &Sections);

        void writePadding(unsigned N);
        void writeELFHeader(const MCAssembler &Asm, const MCAsmLayout &Layout,
            unsigned PHNum, uint64_t &HdrSize);
        void writeProgramHeader(const MCAsmLayout &Layout,
            const MCSectionData &SD, uint64_t &FileOff);
        void writeStrtabSectionHeader();
        void writeProgSectionHeader(const MCAsmLayout &Layout,
            const MCSectionData &SD, uint64_t &FileOff);
        void writeProgramData(MCAssembler &Asm, const MCAsmLayout &Layout,
            const MCSectionData &SD, uint64_t &FileOff);
    };

}  // end namespace

#endif
