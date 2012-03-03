/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "elfdefs.h"
#include "elfutil.h"
#include "flashlayer.h"

#include <string.h>


bool Elf::ProgramInfo::init(const FlashRange &elf)
{
    if (elf.getSize() < FlashBlock::BLOCK_SIZE) {
        LOG(("ELF Error: file is too small\n"));
        return false;
    }
    
    if (!elf.isAligned()) {
        LOG(("ELF Error: file is not block aligned\n"));
        return false;
    }

    FlashBlockRef ref;
    FlashBlock::get(ref, elf.getAddress());
    Elf::FileHeader *header = reinterpret_cast<Elf::FileHeader*>(ref->getData());

    // verify magicality
    if ((header->e_ident[Elf::EI_MAG0] != Elf::Magic0) ||
        (header->e_ident[Elf::EI_MAG1] != Elf::Magic1) ||
        (header->e_ident[Elf::EI_MAG2] != Elf::Magic2) ||
        (header->e_ident[Elf::EI_MAG3] != Elf::Magic3))
    {
        LOG(("ELF Error: incorrect magic number\n"));
        return false;
    }

    // ensure the file is the correct Elf version
    if (header->e_ident[Elf::EI_VERSION] != Elf::EV_CURRENT) {
        LOG(("ELF Error: incorrect file version\n"));
        return false;
    }

    // ensure the file is the correct data format
    union {
        int32_t l;
        char c[sizeof (int32_t)];
    } u;
    u.l = 1;
    if ((u.c[sizeof(int32_t) - 1] + 1) != header->e_ident[Elf::EI_DATA]) {
        LOG(("ELF Error: incorrect data format\n"));
        return false;
    }

    if (header->e_machine != Elf::EM_ARM) {
        LOG(("ELF Error: incorrect machine format\n"));
        return false;
    }
    
    if (header->e_phoff & 3) {
        LOG(("ELF Error: incorrect header alignment\n"));
        return false;
    }
    Elf::ProgramHeader *ph = reinterpret_cast<Elf::ProgramHeader*>
        (header->e_phoff + ref->getData());

    memset(this, 0, sizeof *this);
    entry = header->e_entry;

    /*
        We're looking for 3 segments, identified by the following profiles:
        - text segment - executable & read-only
        - data segment - read/write & non-zero size on disk
        - bss segment - read/write, zero size on disk, and non-zero size in memory
    */

    for (unsigned i = 0; i < header->e_phnum; ++i, ++ph) {
        if (ph->p_type != Elf::PT_LOAD)
            continue;

        switch (ph->p_flags) {

        case (Elf::PF_Exec | Elf::PF_Read):
            rodata.data = elf.split(ph->p_offset, ph->p_filesz);
            if (!rodata.data.isAligned()) {
                LOG(("ELF Error: Flash data segment not aligned\n"));
                return false;
            }
            rodata.vaddr = ph->p_vaddr;
            rodata.size = ph->p_memsz;
            break;

        case (Elf::PF_Read | Elf::PF_Write):
            if (ph->p_memsz && !ph->p_filesz) {
                bss.vaddr = ph->p_vaddr;
                bss.size = ph->p_memsz;
            
            } else if (ph->p_filesz) {
                rwdata.data = elf.split(ph->p_offset, ph->p_filesz);
                rwdata.vaddr = ph->p_vaddr;
                rwdata.size = ph->p_memsz;
            }
            break;
        }
    }

    // RODATA segment is mandatory
    if (rodata.data.isEmpty()) {
        LOG(("ELF Error: No read-only data segment defined\n"));
        return false;
    }

    return true;
}
