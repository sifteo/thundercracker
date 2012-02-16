/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMELFProgramWriter.h"
#include "SVMMCAsmBackend.h"
#include "llvm/Support/CommandLine.h"
using namespace llvm;

cl::opt<bool> ELFDebug("g",
    cl::desc("Include debug information in generated ELF files"));

SVMELFProgramWriter::SVMELFProgramWriter(raw_ostream &OS)
    : MCObjectWriter(OS, true) {}


void SVMELFProgramWriter::WriteObject(MCAssembler &Asm,
    const MCAsmLayout &Layout)
{
    // First pass, allocate all non-debug sections (for symbol table)
    ML.AllocateSections(Asm, Layout);

    if (ELFDebug) {
        // Second pass, allocate all sections
        EMB.BuildSections(Asm, Layout, ML);
        ML.AllocateSections(Asm, Layout);
    }
    
    // Apply fixups that were stored in RecordRelocation
    ML.ApplyLateFixups(Asm, Layout);

    // Write header blocks
    writeELFHeader(Asm, Layout);
    for (int S = 0; S < SPS_DEBUG; ++S)
        writeProgramHeader((SVMProgramSection) S);

    // Write program data, sorted by SPS section
    int endS = ELFDebug ? SPS_NUM_SECTIONS : SPS_DEBUG;
    for (int S = 0; S < endS; ++S) {
        for (MCAssembler::const_iterator it = Asm.begin(), ie = Asm.end();
            it != ie; ++it) {
            const MCSectionData *SD = &*it;

            if (ML.getSectionKind(SD) != S)
                continue;

            if (Layout.getSectionFileSize(SD) == 0)
                continue;

            padToOffset(ML.getSectionDiskOffset(SD, HdrSize));
            Asm.WriteSectionData(SD, Layout);
        }
    }
    
    if (ELFDebug) {
        // On debug binaries, generate section headers last
        padToOffset(SHOffset);
        
        // Dummy NULL section header (index 0)
        WriteZeros(sizeof(ELF::Elf32_Shdr));
        
        for (MCAssembler::const_iterator it = Asm.begin(), ie = Asm.end();
            it != ie; ++it) {
            const MCSectionData *SD = &*it;
            writeSectionHeader(Layout, SD);
        }
    }
}

void SVMELFProgramWriter::writeELFHeader(const MCAssembler &Asm, const MCAsmLayout &Layout)
{
    uint32_t Align = SVMTargetMachine::getBlockSize();

    unsigned PHNum = SPS_DEBUG;
    unsigned PHSize = PHNum * sizeof(ELF::Elf32_Phdr);

    // Debug-only section headers
    unsigned SHNum = ELFDebug ? 1 + Asm.getSectionList().size() : 0;
    unsigned Strtab = ELFDebug ? EMB.getShstrtabIndex() : 0;

    // Place program headers and ELF headers in the proper 'header' block,
    // but keep the section headers at the beginning of the SPS_DEBUG section.
    unsigned PHOffset = sizeof(ELF::Elf32_Ehdr);
    HdrSize = RoundUpToAlignment(PHOffset + PHSize, Align);
    SHOffset = ELFDebug ? ML.getSectionDiskOffset(SPS_END, HdrSize) : 0;
    
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
    Write32(ML.getEntryAddress(Asm, Layout));       // e_entry
    Write32(PHOffset);                              // e_phoff
    Write32(SHOffset);                              // e_shoff
    Write32(0);                                     // e_flags
    Write16(sizeof(ELF::Elf32_Ehdr));               // e_ehsize
    Write16(sizeof(ELF::Elf32_Phdr));               // e_phentsize
    Write16(PHNum);                                 // e_phnum
    Write16(sizeof(ELF::Elf32_Shdr));               // e_shentsize
    Write16(SHNum);                                 // e_shnum
    Write16(Strtab);                                // e_shstrndx
}

void SVMELFProgramWriter::writeProgramHeader(SVMProgramSection S)
{
    /*
     * Program headers are used by the runtime's loader. This is the
     * canonical data used to set up the binary's address space before
     * executing it.
     */

    uint32_t Flags = ELF::PF_R;

    if (S == SPS_RO)
        Flags |= ELF::PF_X;
    else
        Flags |= ELF::PF_W;

    Write32(ELF::PT_LOAD);                          // p_type
    Write32(ML.getSectionDiskOffset(S, HdrSize));   // p_offset
    Write32(ML.getSectionMemAddress(S));            // p_vaddr
    Write32(0);                                     // p_paddr
    Write32(ML.getSectionDiskSize(S));              // p_filesz
    Write32(ML.getSectionMemSize(S));               // p_memsz
    Write32(Flags);                                 // p_flags
    Write32(SVMTargetMachine::getBlockSize());      // p_align
}

void SVMELFProgramWriter::writeSectionHeader(const MCAsmLayout &Layout,
    const MCSectionData *SD)
{
    const MCSectionELF *SE = dyn_cast<MCSectionELF>(&SD->getSection());
    assert(SE);
    
    uint32_t sh_type = SE->getType();
    uint32_t sh_link = 0;
    uint32_t sh_info = 0;

    // Type-specific data finds itself in sh_link and sh_info
    switch (sh_type) {
    case ELF::SHT_SYMTAB:
    case ELF::SHT_DYNSYM:
        sh_link = EMB.getStringTableIndex();
        sh_info = EMB.getLastLocalSymbolIndex();
        break;
    default:
        break;
    }

    Write32(EMB.getSectionStringTableIndex(SE));    // sh_name
    Write32(sh_type);                               // sh_type
    Write32(SE->getFlags());                        // sh_flags
    Write32(ML.getSectionMemAddress(SD));           // sh_addr
    Write32(ML.getSectionDiskOffset(SD, HdrSize));  // sh_offset
    Write32(Layout.getSectionFileSize(SD));         // sh_size
    Write32(sh_link);                               // sh_link
    Write32(sh_info);                               // sh_info
    Write32(SD->getAlignment());                    // sh_addralign
    Write32(SE->getEntrySize());                    // sh_entsize
}

void SVMELFProgramWriter::RecordRelocation(const MCAssembler &Asm,
    const MCAsmLayout &Layout, const MCFragment *Fragment,
    const MCFixup &Fixup, MCValue Target, uint64_t &FixedValue)
{
    ML.RecordRelocation(Asm, Layout, Fragment, Fixup, Target, FixedValue);
}

void SVMELFProgramWriter::writePadding(unsigned N)
{
    const char Pad = SVMTargetMachine::getPaddingByte();
    while (N--)
        OS << StringRef(&Pad, 1);
}

void SVMELFProgramWriter::padToOffset(uint32_t O)
{
    int32_t size = O - OS.tell();
    assert(size >= 0);
    if (size > 0)
        writePadding(size);
    assert(O == OS.tell());
}

MCObjectWriter *llvm::createSVMELFProgramWriter(raw_ostream &OS)
{
    return new SVMELFProgramWriter(OS);
}
