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

#include "SVMELFProgramWriter.h"
#include "SVMMCAsmBackend.h"
#include "SVMTargetMachine.h"
#include "llvm/Support/CommandLine.h"
#include "fastlz.h"
using namespace llvm;

cl::opt<bool> ELFDebug("g",
    cl::desc("Include debug information in generated ELF files"));

SVMELFProgramWriter::SVMELFProgramWriter(raw_ostream &OS)
    : MCObjectWriter(OS, true) {}


void SVMELFProgramWriter::WriteObject(MCAssembler &Asm,
    const MCAsmLayout &Layout)
{
    // First pass, allocate all non-debug sections and compute the
    // initial layout of the plaintext RWDATA segments.
    ML.AllocateSections(Asm, Layout);

    // Apply fixups that were stored in RecordRelocation
    ML.ApplyLateFixups(Asm, Layout);

    // Now we can know the final binary image of the RWDATA segments. Compress them.
    rwCompress(Asm, Layout, ML);
    ML.AllocateSections(Asm, Layout);

    if (ELFDebug) {
        // Allocate all debug sections last
        EMB.BuildSections(Asm, Layout, ML);
        ML.AllocateSections(Asm, Layout);
    }

    // Write header blocks
    writeELFHeader(Asm, Layout);
    for (int S = 0; S < SPS_DEBUG; ++S)
        writeProgramHeader((SVMProgramSection) S);

    // Write program data, sorted by SPS section
    int endS = ELFDebug ? SPS_NUM_SECTIONS : SPS_DEBUG;
    for (int S = 0; S < endS; ++S) {
        
        if (S == SPS_DEBUG)
            writeDebugMessage();
        
        for (MCAssembler::const_iterator it = Asm.begin(), ie = Asm.end();
            it != ie; ++it) {
            const MCSectionData *SD = &*it;

            if (ML.getSectionKind(SD) != S)
                continue;

            if (Layout.getSectionFileSize(SD) == 0)
                continue;

            padToOffset(ML.getSectionDiskOffset(SD));
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
    // Debug-only section headers
    unsigned SHNum = ELFDebug ? 1 + Asm.getSectionList().size() : 0;
    unsigned Strtab = ELFDebug ? EMB.getShstrtabIndex() : 0;
    SHOffset = ELFDebug ?
        RoundUpToAlignment(ML.getSectionDiskOffset(SPS_END), 4) : 0;

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
    Write32(SVMELF::PHOffset);                      // e_phoff
    Write32(SHOffset);                              // e_shoff
    Write32(0);                                     // e_flags
    Write16(sizeof(ELF::Elf32_Ehdr));               // e_ehsize
    Write16(sizeof(ELF::Elf32_Phdr));               // e_phentsize
    Write16(SVMELF::PHNum);                         // e_phnum
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
    uint32_t Type = ELF::PT_LOAD;
    uint32_t Align = sizeof(uint32_t);

    switch (S) {

    case SPS_RO:
        Flags |= ELF::PF_X;
        Align = SVMTargetMachine::getBlockSize();
        break;

    case SPS_BSS:
        Flags |= ELF::PF_W;
        break;

    case SPS_RW_Z:
        Flags |= ELF::PF_W;
        Type = SVMELF::PT_LOAD_FASTLZ;
        break;

    case SPS_META:
        Type = SVMELF::PT_METADATA;
        break;

    default:
        assert(0);
        break;
    }

    Write32(Type);                                  // p_type
    Write32(ML.getSectionDiskOffset(S));            // p_offset
    Write32(ML.getSectionMemAddress(S));            // p_vaddr
    Write32(0);                                     // p_paddr
    Write32(ML.getSectionDiskSize(S));              // p_filesz
    Write32(ML.getSectionMemSize(S));               // p_memsz
    Write32(Flags);                                 // p_flags
    Write32(Align);                                 // p_align
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
    Write32(ML.getSectionDiskOffset(SD));           // sh_offset
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

void SVMELFProgramWriter::padToAlignment(uint32_t A)
{
    int32_t remainder = OS.tell() % A;
    if (remainder != 0)
        writePadding(A - remainder);
    assert(0 == (OS.tell() % A));
}

void SVMELFProgramWriter::writeDebugMessage()
{
    /*
     * When we're linking a debug build, we leave a big obvious warning
     * in the resulting ELF file, to (1) notify folks that this is indeed
     * a debug build, and (2) illustrate the split between non-debug
     * data above, and debug data below.
     *
     * SVMMemoryLayout leaves one block free for this message.
     *
     * We format it in 16-byte-wide lines, so that it shows up clearly
     * in most hex editors.
     */

    padToOffset(ML.getSectionDiskOffset(SPS_DEBUG));
    assert(SVMTargetMachine::getBlockSize() >= 256);

    OS << "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
          "+-----8<------+\n"
          "|             |\n"
          "|  This is a  |\n"
          "| DEBUG BUILD |\n"
          "|             |\n"
          "| All data    |\n"
          "| below this  |\n"
          "| point is    |\n"
          "| only needed |\n"
          "| for debug.  |\n"
          "|             |\n"
          "| /\\/\\/\\/\\/\\/ |\n"
          "|   sproing!  |\n"
          "+-------------+\n"
          "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
}

void SVMELFProgramWriter::rwCompress(MCAssembler &Asm, const MCAsmLayout &Layout, SVMMemoryLayout &ML)
{
    /*
     * Look for all segments with initialized data for RAM, flatten them, and
     * compress the resulting data. Create a new segment with the compressed
     * RWDATA. This segment will always be included in the binary. The originals
     * are considered debug-only sections, since they are no longer needed
     * at runtime.
     */

    // Flattened binary contents of RAM
    std::vector<uint8_t> plaintext;

    // Iterate over SPS_RW sections
    for (MCAssembler::const_iterator IS = Asm.begin(), ES = Asm.end(); IS != ES; ++IS) {
        const MCSectionData *SD = &*IS;
        if (ML.getSectionKind(SD) != SPS_RW_PLAIN)
            continue;

        int offset = ML.getSectionMemAddress(SD) - SVMTargetMachine::getRAMBase();
        int limit = SVMTargetMachine::getRAMSize();

        // Iterate over fragments, pasting them into 'plaintext'
        for (MCSectionData::const_iterator IF = SD->begin(), EF = SD->end(); IF != EF; ++IF) {
            const MCFragment *F = &*IF;
            if (F->getKind() == MCFragment::FT_Data) {
                const MCDataFragment *DF = cast<MCDataFragment>(F);
                int fragmentOffset = Layout.getFragmentOffset(F);
                for (unsigned i = 0; i < DF->getContents().size(); i++) {
                    int totalOffset = fragmentOffset + offset + i;
                    if (totalOffset < limit) {
                        while (totalOffset >= int(plaintext.size()))
                            plaintext.push_back(0);
                        plaintext[totalOffset] = DF->getContents()[i];
                    }
                }
            }
        }
    }

    // FastLZ requires a minimum of 16 bytes to compress. Pad our section data.
    while (plaintext.size() < 16)
        plaintext.push_back(0);

    // Compress using FastLZ level 1
    std::vector<uint8_t> compressed(plaintext.size() * 2);
    compressed.resize(fastlz_compress_level(1, &plaintext[0], plaintext.size(), &compressed[0]));

    // Create the new section
    const MCSectionELF *LZSection =
        Asm.getContext().getELFSection(".rwdata.lz", ELF::SHT_NOTE,
            0, SectionKind::getDataNoRel());
    MCSectionData &LZSectionSD = Asm.getOrCreateSectionData(*LZSection);
    LZSectionSD.setAlignment(1);

    // Force it to be interpreted as SPS_RW, and set the decompressed size
    ML.setSectionKind(&LZSectionSD, SPS_RW_Z);
    ML.setSectionMemSize(&LZSectionSD, plaintext.size());

    // Add compressed data
    MCDataFragment *F = new MCDataFragment(&LZSectionSD);
    F->getContents().append(compressed.begin(), compressed.end());
}

MCObjectWriter *llvm::createSVMELFProgramWriter(raw_ostream &OS)
{
    return new SVMELFProgramWriter(OS);
}
