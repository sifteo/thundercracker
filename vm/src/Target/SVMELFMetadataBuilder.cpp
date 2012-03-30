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

#include "SVMELFMetadataBuilder.h"
#include "SVMMemoryLayout.h"
#include "llvm/Support/ErrorHandling.h"
using namespace llvm;


static int compareBySuffix(const void *a, const void *b)
{
    const MCSectionELF *secA = *static_cast<const MCSectionELF* const *>(a);
    const MCSectionELF *secB = *static_cast<const MCSectionELF* const *>(b);
    const StringRef &NameA = secA->getSectionName();
    const StringRef &NameB = secB->getSectionName();
    const unsigned sizeA = NameA.size();
    const unsigned sizeB = NameB.size();
    const unsigned len = std::min(sizeA, sizeB);
    for (unsigned int i = 0; i < len; ++i) {
        char ca = NameA[sizeA - i - 1];
        char cb = NameB[sizeB - i - 1];
        if (ca != cb)
            return cb - ca;
    }

    return sizeB - sizeA;
}

static void StringLE16(char *buf, uint16_t Value)
{
    buf[0] = char(Value >> 0);
    buf[1] = char(Value >> 8);
}

static void StringLE32(char *buf, uint32_t Value)
{
    StringLE16(buf, uint16_t(Value >> 0));
    StringLE16(buf + 2, uint16_t(Value >> 16));
}

static void String8(MCDataFragment &F, uint8_t Value)
{
    char buf[1];
    buf[0] = Value;
    F.getContents() += StringRef(buf, 1);
}

static void String16(MCDataFragment &F, uint16_t Value)
{
    char buf[2];
    StringLE16(buf, Value);
    F.getContents() += StringRef(buf, 2);
}

void String32(MCDataFragment &F, uint32_t Value)
{
    char buf[4];
    StringLE32(buf, Value);
    F.getContents() += StringRef(buf, 4);
}

void SVMELFMetadataBuilder::BuildSections(MCAssembler &Asm,
    const MCAsmLayout &Layout, const SVMMemoryLayout &ML)
{
    MCContext &Ctx = Asm.getContext();
    MCDataFragment *F;

    // We construct .shstrtab, .symtab and .strtab in this order to match gnu as.
    const MCSectionELF *ShstrtabSection =
        Ctx.getELFSection(".shstrtab", ELF::SHT_STRTAB, 0,
            SectionKind::getReadOnly());
    MCSectionData &ShstrtabSD = Asm.getOrCreateSectionData(*ShstrtabSection);
    ShstrtabSD.setAlignment(1);

    const MCSectionELF *SymtabSection =
        Ctx.getELFSection(".symtab", ELF::SHT_SYMTAB, 0,
            SectionKind::getReadOnly(),
            ELF::SYMENTRY_SIZE32, "");
    MCSectionData &SymtabSD = Asm.getOrCreateSectionData(*SymtabSection);
    SymtabSD.setAlignment(4);

    const MCSectionELF *StrtabSection;
    StrtabSection = Ctx.getELFSection(".strtab", ELF::SHT_STRTAB, 0,
        SectionKind::getReadOnly());
    MCSectionData &StrtabSD = Asm.getOrCreateSectionData(*StrtabSection);
    StrtabSD.setAlignment(1);

    SectionIndexMapTy SectionIndexMap;
    ComputeIndexMap(Asm, SectionIndexMap);
    ComputeSymbolTable(Asm, SectionIndexMap);

    ShstrtabIndex = SectionIndexMap.lookup(ShstrtabSection);
    SymbolTableIndex = SectionIndexMap.lookup(SymtabSection);
    StringTableIndex = SectionIndexMap.lookup(StrtabSection);

    // Symbol table
    F = new MCDataFragment(&SymtabSD);
    WriteSymbolTable(F, Asm, Layout, SectionIndexMap, ML);

    F = new MCDataFragment(&StrtabSD);
    F->getContents().append(StringTable.begin(), StringTable.end());

    F = new MCDataFragment(&ShstrtabSD);

    std::vector<const MCSectionELF*> Sections;
    for (MCAssembler::const_iterator it = Asm.begin(),
           ie = Asm.end(); it != ie; ++it) {
        const MCSectionELF &Section =
            static_cast<const MCSectionELF&>(it->getSection());
        Sections.push_back(&Section);
    }
    array_pod_sort(Sections.begin(), Sections.end(), compareBySuffix);

    // Section header string table.
    //
    // The first entry of a string table holds a null character so skip
    // section 0.
    uint64_t Index = 1;
    F->getContents() += '\x00';

    for (unsigned int I = 0, E = Sections.size(); I != E; ++I) {
        const MCSectionELF &Section = *Sections[I];

        StringRef Name = Section.getSectionName();
        if (I != 0) {
            StringRef PreviousName = Sections[I - 1]->getSectionName();
            if (PreviousName.endswith(Name)) {
                SectionStringTableIndex[&Section] = Index - Name.size() - 1;
                continue;
            }
        }
        // Remember the index into the string table so we can write it
        // into the sh_name field of the section header table.
        SectionStringTableIndex[&Section] = Index;

        Index += Name.size() + 1;
        F->getContents() += Name;
        F->getContents() += '\x00';
    }
}

bool SVMELFMetadataBuilder::isInSymtab(const MCAssembler &Asm, const MCSymbolData &Data)
{
    const MCSymbol &Symbol = Data.getSymbol();
    StringRef Name = Symbol.getName();

    if (!Symbol.isInSection())
        return false;

    if (Symbol.isTemporary())
        return false;
        
    // Ignore internal "decorated" symbols
    if (Name.startswith("_") && Name.count("$"))
        return false;

    return true;
}

void SVMELFMetadataBuilder::ComputeSymbolTable(MCAssembler &Asm,
    const SectionIndexMapTy &SectionIndexMap)
{
    // Index 0 is always the empty string.
    StringMap<uint64_t> StringIndexMap;
    StringTable += '\x00';

    // Add the data for the symbols.
    for (MCAssembler::symbol_iterator it = Asm.symbol_begin(),
        ie = Asm.symbol_end(); it != ie; ++it) {
        const MCSymbol &Symbol = it->getSymbol();

        if (!isInSymtab(Asm, *it))
            continue;

        ELFSymbolData MSD;
        MSD.SymbolData = it;
        const MCSymbol &RefSymbol = Symbol.AliasedSymbol();

        const MCSectionELF &Section =
            static_cast<const MCSectionELF&>(RefSymbol.getSection());
        MSD.SectionIndex = SectionIndexMap.lookup(&Section);
        assert(MSD.SectionIndex && "Invalid section index!");

        StringRef Name = Symbol.getName();
        uint64_t &Entry = StringIndexMap[Name];
        if (!Entry) {
            Entry = StringTable.size();
            StringTable += Name;
            StringTable += '\x00';
        }
        MSD.StringIndex = Entry;

        LocalSymbolData.push_back(MSD);
    }

    // Symbols are required to be in lexicographic order.
    array_pod_sort(LocalSymbolData.begin(), LocalSymbolData.end());

    unsigned Index = 1;
    for (unsigned i = 0, e = LocalSymbolData.size(); i != e; ++i)
        LocalSymbolData[i].SymbolData->setIndex(Index++);
}

void SVMELFMetadataBuilder::ComputeIndexMap(MCAssembler &Asm,
    SectionIndexMapTy &SectionIndexMap)
{
    unsigned Index = 1;
    for (MCAssembler::iterator it = Asm.begin(),
        ie = Asm.end(); it != ie; ++it) {
        const MCSectionELF &Section =
            static_cast<const MCSectionELF &>(it->getSection());
        SectionIndexMap[&Section] = Index++;
    }
}

void SVMELFMetadataBuilder::WriteSymbolTable(MCDataFragment *SymtabF,
    const MCAssembler &Asm,  const MCAsmLayout &Layout,
    const SectionIndexMapTy &SectionIndexMap, const SVMMemoryLayout &ML)
{
    // The string table must be emitted first because we need the index
    // into the string table for all the symbol names.
    assert(StringTable.size() && "Missing string table");

    // The first entry is the undefined symbol entry.
    WriteSymbolEntry(SymtabF, 0, 0, 0, 0, 0, 0);

    // Write the symbol table entries.
    for (unsigned i = 0, e = LocalSymbolData.size(); i != e; ++i) {
        ELFSymbolData &MSD = LocalSymbolData[i];
        WriteSymbol(SymtabF, MSD, Asm, Layout, ML);
    }
}

void SVMELFMetadataBuilder::WriteSymbolEntry(MCDataFragment *SymtabF,
    uint32_t name, uint8_t info, uint32_t value,
    uint32_t size, uint8_t other, uint32_t shndx)
{
    String32(*SymtabF, name);  // st_name
    String32(*SymtabF, value); // st_value
    String32(*SymtabF, size);  // st_size
    String8(*SymtabF, info);   // st_info
    String8(*SymtabF, other);  // st_other
    String16(*SymtabF, shndx); // st_shndx
}

void SVMELFMetadataBuilder::WriteSymbol(MCDataFragment *SymtabF,
    ELFSymbolData &MSD, const MCAssembler &Asm, const MCAsmLayout &Layout,
    const SVMMemoryLayout &ML)
{
    MCSymbolData &OrigData = *MSD.SymbolData;
    MCSymbolData &Data =
        Layout.getAssembler().getSymbolData(OrigData.getSymbol().AliasedSymbol());

    uint8_t Binding = MCELF::GetBinding(OrigData);
    uint8_t Visibility = MCELF::GetVisibility(OrigData);
    uint8_t Type = MCELF::GetType(Data);

    uint8_t Info = (Binding << ELF_STB_Shift) | (Type << ELF_STT_Shift);
    uint8_t Other = Visibility;

    uint32_t Value = SymbolValue(Data, Asm, Layout, ML);
    uint32_t Size = 0;

    assert(!(Data.isCommon() && !Data.isExternal()));

    const MCExpr *ESize = Data.getSize();
    if (ESize) {
        int64_t Res;
        if (!ESize->EvaluateAsAbsolute(Res, Layout))
            report_fatal_error("Size expression must be absolute.");
        Size = Res;
    }

    // Write out the symbol table entry
    WriteSymbolEntry(SymtabF, MSD.StringIndex, Info, Value,
                    Size, Other, MSD.SectionIndex);
}

uint32_t SVMELFMetadataBuilder::SymbolValue(MCSymbolData &Data,
    const MCAssembler &Asm, const MCAsmLayout &Layout,
    const SVMMemoryLayout &ML)
{
    if (Data.isCommon() && Data.isExternal())
        return Data.getCommonAlignment();

    const MCSymbol &Symbol = Data.getSymbol();

    if (Symbol.isAbsolute() && Symbol.isVariable()) {
        if (const MCExpr *Value = Symbol.getVariableValue()) {
            int64_t IntValue;
            if (Value->EvaluateAsAbsolute(IntValue, Layout))
	            return (uint64_t)IntValue;
        }
    }

    assert(Symbol.isInSection());

    /*
     * For normal symbols, executable ELFs (as opposed to relocatable)
     * expect a full virtual address for symbols, not a section-relative
     * offset. This is a job for SVMMemoryLayout.
     *
     * Note that we disable code-address generation. Unlike all addresses
     * we use in actual code or in describing entry points, the debug symbols
     * are always treated as pure virtual addresses, never decorated with SP
     * adjustments or other flags.
     */

    if (Data.getFragment()) {
        SVMSymbolInfo SI = ML.getSymbol(Asm, Layout, &Symbol, false);

        if (Data.getFlags() & ELF_Other_ThumbFunc)
            return SI.Value + 1;
        else
            return SI.Value;
    }

    return 0;
}
