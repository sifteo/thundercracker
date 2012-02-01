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
 */

#include "SVM.h"
#include "SVMMCTargetDesc.h"
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
using namespace llvm;

namespace {
    
typedef std::vector<const MCSectionData*> SectionDataList;    
    
class SVMELFProgramWriter : public MCObjectWriter {
public:    
    SVMELFProgramWriter(raw_ostream &OS)
        : MCObjectWriter(OS, true) {}

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
        uint64_t &FixedValue)
    {
        // Nothing to do
    }

    void WriteObject(MCAssembler &Asm, const MCAsmLayout &Layout);

private:
    unsigned GetEntryAddress();
    void CollectProgramSections(MCAssembler &Asm, SectionDataList &Sections);

    void WritePadding(unsigned N);
    void WriteELFHeader(unsigned Entry, unsigned PHNum, uint64_t &HdrSize);
    void WriteProgramHeader(const MCAsmLayout &Layout,
        const MCSectionData &SD, uint64_t &FileOff);
    void WriteProgSectionHeader(const MCAsmLayout &Layout,
        const MCSectionData &SD, uint64_t &FileOff);
    void WriteProgramData(MCAssembler &Asm, const MCAsmLayout &Layout,
        const MCSectionData &SD, uint64_t &FileOff);
};

}  // end namespace


void SVMELFProgramWriter::WritePadding(unsigned N)
{
    /*
     * Instead of padding with zeroes, pad with ones.
     * These programs get stored in flash memory, and 0xFF
     * is what the flash erases to.
     */

    const char Ones[16] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    };

    for (unsigned i = 0, e = N / 16; i != e; ++i)
        OS << StringRef(Ones, 16);

    OS << StringRef(Ones, N % 16);
}

void SVMELFProgramWriter::WriteELFHeader(unsigned Entry, unsigned PHNum,
    uint64_t &HdrSize)
{
    unsigned PHSize = PHNum * sizeof(ELF::Elf32_Phdr);
    unsigned SHSize = PHNum * sizeof(ELF::Elf32_Shdr);
    HdrSize = sizeof(ELF::Elf32_Ehdr) + PHSize + SHSize;
    
    unsigned PHOff = sizeof(ELF::Elf32_Ehdr);
    unsigned SHOff = PHOff + PHSize;
    
    Write8(0x7f);                                   // e_ident
    Write8('E');
    Write8('L');
    Write8('F');
    Write8(ELF::ELFCLASS32);
    Write8(ELF::ELFDATA2LSB);
    Write8(ELF::EV_CURRENT);
    Write8(ELF::ELFOSABI_NONE);
    Write8(0);                                      // ABI Version
    WriteZeros(ELF::EI_NIDENT - ELF::EI_PAD);
    
    Write16(ELF::ET_EXEC);                          // e_type
    Write16(ELF::EM_ARM);                           // e_machine
    Write32(ELF::EV_CURRENT);                       // e_version
    Write32(Entry);                                 // e_entry
    Write32(PHOff);                                 // e_phoff
    Write32(SHOff);                                 // e_shoff
    Write32(0);                                     // e_flags
    Write16(sizeof(ELF::Elf32_Ehdr));               // e_ehsize
    Write16(sizeof(ELF::Elf32_Phdr));               // e_phentsize
    Write16(PHNum);                                 // e_phnum
    Write16(sizeof(ELF::Elf32_Shdr));               // e_shentsize
    Write16(PHNum);                                 // e_shnum
    Write16(0);                                     // e_shstrndx
}

void SVMELFProgramWriter::WriteProgramHeader(const MCAsmLayout &Layout,
    const MCSectionData &SD, uint64_t &FileOff)
{
    /*
     * Program headers are used by the runtime's loader. This is the
     * canonical data used to set up the binary's address space before
     * executing it.
     */
    
    uint64_t AddressSize = Layout.getSectionAddressSize(&SD);
    uint64_t FileSize = Layout.getSectionFileSize(&SD);
    unsigned Alignment = SD.getAlignment();
    unsigned VirtualAddr = 0x1234;
    SectionKind Kind = SD.getSection().getKind();
    uint32_t Flags = ELF::PF_R;

    if (Kind.isText())                  Flags |= ELF::PF_X;
    if (Kind.isGlobalWriteableData())   Flags |= ELF::PF_W;

    FileOff = RoundUpToAlignment(FileOff, Alignment);

    Write32(ELF::PT_LOAD);                          // p_type
    Write32(FileOff);                               // p_offset
    Write32(VirtualAddr);                           // p_vaddr
    Write32(0);                                     // p_paddr
    Write32(FileSize);                              // p_filesz
    Write32(AddressSize);                           // p_memsz
    Write32(Flags);                                 // p_flags
    Write32(Alignment);                             // p_align

    FileOff += FileSize;
}

void SVMELFProgramWriter::WriteProgSectionHeader(const MCAsmLayout &Layout,
    const MCSectionData &SD, uint64_t &FileOff)
{
    /*
     * Section headers are used only by debug tools, not by the runtime.
     * For convenience, we create a set of section headers corresponding
     * to each program header.
     */
    
    uint64_t FileSize = Layout.getSectionFileSize(&SD);
    unsigned Alignment = SD.getAlignment();
    unsigned VirtualAddr = 0x1234;
    SectionKind Kind = SD.getSection().getKind();
    uint32_t Flags = ELF::SHF_ALLOC;

    FileOff = RoundUpToAlignment(FileOff, Alignment);

    if (Kind.isText())                  Flags |= ELF::SHF_EXECINSTR;
    if (Kind.isGlobalWriteableData())   Flags |= ELF::SHF_WRITE;

    Write32(0);                                     // sh_name
    Write32(ELF::SHT_PROGBITS);                     // sh_type
    Write32(Flags);                                 // sh_flags
    Write32(VirtualAddr);                           // sh_addr
    Write32(FileOff);                               // sh_offset
    Write32(FileSize);                              // sh_size
    Write32(0);                                     // sh_link
    Write32(0);                                     // sh_info
    Write32(Alignment);                             // sh_addralign
    Write32(0);                                     // sh_entsize

    FileOff += FileSize;
}
void SVMELFProgramWriter::WriteProgramData(MCAssembler &Asm,
    const MCAsmLayout &Layout, const MCSectionData &SD, uint64_t &FileOff)
{
    unsigned Alignment = SD.getAlignment();
    unsigned Padding = OffsetToAlignment(FileOff, Alignment);
    
    WritePadding(Padding);
    FileOff += Padding;
    FileOff += Layout.getSectionFileSize(&SD);

    Asm.WriteSectionData(&SD, Layout);
}   

void SVMELFProgramWriter::CollectProgramSections(MCAssembler &Asm, SectionDataList &Sections)
{
    // All text sections
    for (MCAssembler::iterator it = Asm.begin(), ie = Asm.end(); it != ie; ++it) {
        SectionKind k = it->getSection().getKind();
        if (k.isText())
            Sections.push_back(&*it);
    }

    // All read-only data
    for (MCAssembler::iterator it = Asm.begin(), ie = Asm.end(); it != ie; ++it) {
        SectionKind k = it->getSection().getKind();
        if (k.isReadOnly())
            Sections.push_back(&*it);
    }
    
    // All RAM data
    for (MCAssembler::iterator it = Asm.begin(), ie = Asm.end(); it != ie; ++it) {
        SectionKind k = it->getSection().getKind();
        if (k.isGlobalWriteableData() && !k.isBSS())
            Sections.push_back(&*it);
    }
}

unsigned SVMELFProgramWriter::GetEntryAddress()
{
    // XXX
    return 0x12345678;
}

void SVMELFProgramWriter::WriteObject(MCAssembler &Asm, const MCAsmLayout &Layout)
{
    SectionDataList ProgSections;
    CollectProgramSections(Asm, ProgSections);
    
    uint64_t FileOff, HdrSize;
    WriteELFHeader(GetEntryAddress(), ProgSections.size(), HdrSize);
    
    FileOff = HdrSize;
    for (SectionDataList::iterator it = ProgSections.begin(), ie = ProgSections.end();
        it != ie; ++it)
        WriteProgramHeader(Layout, **it, FileOff);

    FileOff = HdrSize;
    for (SectionDataList::iterator it = ProgSections.begin(), ie = ProgSections.end();
        it != ie; ++it)
        WriteProgSectionHeader(Layout, **it, FileOff);

    FileOff = HdrSize;
    for (SectionDataList::iterator it = ProgSections.begin(), ie = ProgSections.end();
        it != ie; ++it)
        WriteProgramData(Asm, Layout, **it, FileOff);
}


MCObjectWriter *llvm::createSVMELFProgramWriter(raw_ostream &OS)
{
    return new SVMELFProgramWriter(OS);
}
