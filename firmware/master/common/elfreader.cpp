#include "elfreader.h"
#include "elfdefs.h"
#include "flashlayer.h"

#include <string.h>

ElfReader::ElfReader()
{
}

void ElfReader::load(unsigned addr, unsigned len)
{
    unsigned sz;
    void *b = FlashLayer::getRegionFromOffset(addr, FlashLayer::BLOCK_SIZE, &sz);
    uint8_t *block = static_cast<uint8_t*>(b);

    // verify magicality
    if ((block[Elf::EI_MAG0] != Elf::Magic0) ||
        (block[Elf::EI_MAG1] != Elf::Magic1) ||
        (block[Elf::EI_MAG2] != Elf::Magic2) ||
        (block[Elf::EI_MAG3] != Elf::Magic3))
    {
        LOG(("incorrect elf magic number"));
        return;
    }


    Elf::FileHeader header;
    memcpy(&header, block, sizeof header);

    // ensure the file is the correct data format
    union {
        int32_t l;
        char c[sizeof (int32_t)];
    } u;
    u.l = 1;
    if ((u.c[sizeof(int32_t) - 1] + 1) != header.e_ident[Elf::EI_DATA]) {
        LOG(("incorrect elf data format\n"));
        return;
    }

    if (header.e_machine != Elf::EM_ARM) {
        LOG(("elf: incorrect machine format\n"));
        return;
    }

    dumpHeader(header);


    unsigned offset = header.e_phoff;
    for (unsigned i = 0; i < header.e_phnum; ++i, offset += header.e_phentsize) {
        Elf::ProgramHeader pHeader;
        memcpy(&pHeader, block + offset, header.e_phentsize);
        dumpProgramHeader(pHeader);
    }

    offset = header.e_shoff;
    for (unsigned i = 0; i < header.e_shnum; ++i, offset += header.e_shentsize) {
        Elf::SectionHeader sHeader;
        memcpy(&sHeader, block + offset, header.e_shentsize);
        dumpSectionHeader(sHeader);
    }

    FlashLayer::releaseRegionFromOffset(addr);
}

void ElfReader::dumpHeader(const Elf::FileHeader & header)
{
    LOG(("Elf hdr, size: 0x%x\n", header.e_ehsize));
    LOG(("Elf hdr, file type: 0x%x\n", header.e_type));
    LOG(("Elf hdr, entry point: 0x%x\n", header.e_entry));
    LOG(("Elf hdr, program header table offset: 0x%x\n", header.e_phoff));
    LOG(("Elf hdr, section header table offset: 0x%x\n", header.e_shoff));
    LOG(("Elf hdr, num program header table entries: 0x%x\n", header.e_phnum));
    LOG(("Elf hdr, program header table entry size: 0x%x\n", header.e_phentsize));
    LOG(("Elf hdr, num sections: 0x%x\n", header.e_shnum));
    LOG(("Elf hdr, section header table entry size: 0x%x\n", header.e_shentsize));
    LOG(("Elf hdr, section header string table index: 0x%x\n\n", header.e_shstrndx));
}

void ElfReader::dumpProgramHeader(const Elf::ProgramHeader & header)
{
    LOG(("*************************************** program header\n"));
    LOG(("pgm hdr, type: %d\n", header.p_type));
    LOG(("pgm hdr, offset: 0x%x\n", header.p_offset));
    LOG(("pgm hdr, virtual address: 0x%x\n", header.p_vaddr));
    LOG(("pgm hdr, physical address: 0x%x\n", header.p_paddr));
    LOG(("pgm hdr, filesz: 0x%x\n", header.p_filesz));
    LOG(("pgm hdr, memsz: 0x%x\n", header.p_memsz));
    LOG(("pgm hdr, flags: 0x%x\n", header.p_flags));
    LOG(("pgm hdr, align: 0x%x\n\n", header.p_align));
}

void ElfReader::dumpSectionHeader(const Elf::SectionHeader & header)
{
    LOG(("*************************************** section header\n"));
    LOG(("section, name: 0x%x\n", header.sh_name));
    LOG(("section, type: 0x%x\n", header.sh_type));
    LOG(("section, flags: 0x%x\n", header.sh_flags));
    LOG(("section, address: 0x%x\n", header.sh_addr));
    LOG(("section, offset: 0x%x\n", header.sh_offset));
    LOG(("section, size: 0x%x\n", header.sh_size));
    LOG(("section, link: 0x%x\n", header.sh_link));
    LOG(("section, info: 0x%x\n", header.sh_info));
    LOG(("section, address align: 0x%x\n", header.sh_addralign));
    LOG(("section, entry size: 0x%x\n\n", header.sh_entsize));
}
