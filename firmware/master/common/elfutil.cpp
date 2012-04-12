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
     * We're looking for up to 4 segments, identified by the following profiles:
     *  - text segment - executable & read-only
     *  - data segment - read/write & non-zero size on disk
     *  - bss segment - read/write, zero size on disk, and non-zero size in memory
     *  - metadata segment - custom program header type
     */

    for (unsigned i = 0; i < header->e_phnum; ++i, ++ph) {
        FlashRange segData = elf.split(ph->p_offset, ph->p_filesz);
        switch (ph->p_type) {

        case _SYS_ELF_PT_METADATA:
            if (!segData.isAligned(sizeof(uint32_t))) {
                LOG(("ELF Error: Metadata segment not aligned\n"));
                return false;
            }            
            meta.data = segData;
            break;

        case Elf::PT_LOAD:
            switch (ph->p_flags) {

            case (Elf::PF_Exec | Elf::PF_Read):
                if (!segData.isAligned()) {
                    LOG(("ELF Error: Flash data segment not aligned\n"));
                    return false;
                }
                rodata.data = segData;
                rodata.vaddr = ph->p_vaddr;
                rodata.size = ph->p_memsz;
                break;

            case (Elf::PF_Read | Elf::PF_Write):
                if (ph->p_memsz && !ph->p_filesz) {
                    bss.vaddr = ph->p_vaddr;
                    bss.size = ph->p_memsz;
            
                } else if (ph->p_filesz) {
                    rwdata.data = segData;
                    rwdata.vaddr = ph->p_vaddr;
                    rwdata.size = ph->p_memsz;
                }
                break;
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

const char *Elf::Metadata::getString(FlashBlockRef &ref, uint16_t key) const
{
    /*
     * Get a NUL-terminated string metadata key. Verifies that the string
     * actually contains a NUL terminator within the actual size of the
     * metadata record. If the metadata key doesn't exist or it isn't
     * a properly terminated string, we return 0.
     */

    uint32_t actualSize;
    const void *data = get(ref, key, 0, actualSize);

    if (data && memchr(data, 0, actualSize))
        return (const char *) data;
    else
        return 0;
}

const void *Elf::Metadata::get(FlashBlockRef &ref, uint16_t key, uint32_t size) const
{
    /*
     * Convenience method for metadata accessors that only care about a minimum
     * size; they don't need to see the actual size of the metadata record.
     * This is the common case for metadata that's treated as a scalar value
     * instead of an array.
     */

    uint32_t actualSize;
    return get(ref, key, size, actualSize);
}

const void *Elf::Metadata::get(FlashBlockRef &ref, uint16_t key,
            uint32_t minSize, uint32_t &actualSize) const
{
    /*
     * Base accessor for metadata. Looks for 'key' within the metadata
     * segment. If the key exists, is properly aligned, and at least
     * 'minSize' bytes of data are valid, we return a pointer to the
     * key within the flash block cache. If any of these requirements
     * are not met, returns 0.
     *
     * On success, we return the actual size of the key via 'actualSize'.
     * This size includes any padding inserted by the compiler after the
     * actual key data, but it also may be used to implement variable-sized
     * metadata records.
     */

    /*
     * The metadata segment begins with an array of _SYSMetadataKey structs.
     * Note that ProgramInfo::init has already verified the 32-bit alignment
     * of this segment.
     *
     * We need to scan through all the keys, so that we know where the values
     * start. Then we can calculate the proper address of the value we want,
     * without scanning the value array.
     */

    bool foundKey = false;
    uint32_t keyOffset = 0;

    for (uint32_t I = 0, E = data.getSize() - sizeof(_SYSMetadataKey);
        I <= E; I += sizeof(_SYSMetadataKey)) {

        const _SYSMetadataKey *record =
            FlashBlock::getValue<_SYSMetadataKey>(ref, data.getAddress() + I);

        bool isLast = record->stride >> 15;
        uint16_t stride = record->stride & 0x7FFF;

        if (record->key == key) {
            actualSize = stride;
            foundKey = true;
        } else if (!foundKey) {
            keyOffset += stride;
        }

        if (isLast) {
            // Key was missing?
            if (!foundKey)
                return 0;

            // Now we can calculate the address of the value, yay.
            uint32_t valueAddr = data.getAddress() + I + sizeof(_SYSMetadataKey) + keyOffset;
            uint8_t *value = FlashBlock::getBytes(ref, valueAddr, actualSize);

            // Too small, or hitting the edge of the block?
            if (actualSize < minSize)
                return 0;

            return value;
        }
    }
    
    // Never found the "last key" marker :(
    return 0;
}
