/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "elfdefs.h"
#include "elfprogram.h"
#include <string.h>
#include <algorithm>


bool Elf::Program::init(const FlashMapSpan &span)
{
    this->span = span;
    FlashBlockRef ref;

    const Elf::FileHeader *header = getFileHeader(ref);
    if (!header) {
        LOG(("ELF Error: can't read header\n"));
        return false;
    }

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

    // RODATA segment is mandatory
    if (!getRODataSegment(ref)) {
        LOG(("ELF Error: No valid read-only data segment defined\n"));
        return false;
    }

    return true;
}

const Elf::FileHeader *Elf::Program::getFileHeader(FlashBlockRef &ref) const
{
    if (span.getBlock(ref, 0))
        return reinterpret_cast<const FileHeader*>(ref->getData());
    return 0;
}

const Elf::ProgramHeader *Elf::Program::getProgramHeader(FlashBlockRef &ref, unsigned index) const
{
    return getProgramHeader(getFileHeader(ref), index);
}

const Elf::ProgramHeader *Elf::Program::getProgramHeader(const FileHeader *fh, unsigned index) const
{
    ASSERT(fh);
    ASSERT(index < fh->e_phnum);
    unsigned offset = fh->e_phoff + index * sizeof(ProgramHeader);

    if (offset + sizeof(ProgramHeader) > FlashBlock::BLOCK_SIZE)
        return 0;
    else
        return reinterpret_cast<Elf::ProgramHeader*>((uint8_t*)fh + offset);
}

const Elf::ProgramHeader *Elf::Program::getMetadataSegment(FlashBlockRef &ref) const
{
    const FileHeader *fh = getFileHeader(ref);
    ASSERT(fh);

    for (unsigned i = 0; i < fh->e_phnum; ++i) {
        const ProgramHeader *ph = getProgramHeader(fh, i);
        if (!ph)
            break;

        // Segment must be of the proper type, and 32-bit aligned.
        if (ph->p_type == _SYS_ELF_PT_METADATA && (ph->p_offset & 3) == 0)
            return ph;
    }
    return 0;
}

const Elf::ProgramHeader *Elf::Program::getRODataSegment(FlashBlockRef &ref) const
{
    const FileHeader *fh = getFileHeader(ref);
    ASSERT(fh);

    for (unsigned i = 0; i < fh->e_phnum; ++i) {
        const ProgramHeader *ph = getProgramHeader(fh, i);
        if (!ph)
            break;

        // Looking for a block-aligned executable segment.
        if (ph->p_type == Elf::PT_LOAD &&
            ph->p_flags == (Elf::PF_Exec | Elf::PF_Read) &&
            (ph->p_offset & FlashBlock::BLOCK_MASK) == 0)
            return ph;
    }

    return 0;
}

const Elf::ProgramHeader *Elf::Program::getRWDataSegment(FlashBlockRef &ref) const
{
    const FileHeader *fh = getFileHeader(ref);
    ASSERT(fh);

    for (unsigned i = 0; i < fh->e_phnum; ++i) {
        const ProgramHeader *ph = getProgramHeader(fh, i);
        if (!ph)
            break;

        // Looking for a read/write segment with non-zero size on disk
        if (ph->p_flags == (Elf::PF_Write | Elf::PF_Read) &&
            ph->p_filesz != 0)
            return ph;
    }

    return 0;
}

const uint32_t Elf::Program::getTopOfRAM() const
{
    // Look up the first unused RAM address

    FlashBlockRef ref;
    uint32_t top = 0;
    const FileHeader *fh = getFileHeader(ref);
    ASSERT(fh);

    for (unsigned i = 0; i < fh->e_phnum; ++i) {
        const ProgramHeader *ph = getProgramHeader(fh, i);
        if (!ph)
            break;

        if (ph->p_type == Elf::PT_LOAD &&
            ph->p_flags == (Elf::PF_Write | Elf::PF_Read))
            top = std::max(top, ph->p_vaddr + ph->p_memsz);
    }

    return top;
}

uint32_t Elf::Program::getEntry() const
{
    FlashBlockRef ref;
    return getFileHeader(ref)->e_entry;
}

const FlashMapSpan Elf::Program::getRODataSpan() const
{
    FlashBlockRef ref;
    const ProgramHeader *ph = getRODataSegment(ref);
    ASSERT(ph);
    return span.splitRoundingUp(ph->p_offset, ph->p_filesz);
}

const char *Elf::Program::getMetaString(FlashBlockRef &ref, uint16_t key) const
{
    /*
     * Get a NUL-terminated string metadata key. Verifies that the string
     * actually contains a NUL terminator within the actual size of the
     * metadata record. If the metadata key doesn't exist or it isn't
     * a properly terminated string, we return 0.
     */

    uint32_t actualSize;
    const void *data = getMeta(ref, key, 0, actualSize);

    if (data && memchr(data, 0, actualSize))
        return (const char *) data;
    else
        return 0;
}

const void *Elf::Program::getMeta(FlashBlockRef &ref, uint16_t key, uint32_t size) const
{
    /*
     * Convenience method for metadata accessors that only care about a minimum
     * size; they don't need to see the actual size of the metadata record.
     * This is the common case for metadata that's treated as a scalar value
     * instead of an array.
     */

    uint32_t actualSize;
    return getMeta(ref, key, size, actualSize);
}

const void *Elf::Program::getMeta(FlashBlockRef &ref, uint16_t key,
            uint32_t minSize, uint32_t &actualSize) const
{
    uint32_t offset = getMetaSpanOffset(ref, key, minSize, actualSize);
    FlashMapSpan::PhysAddr valuePA;

    if (offset &&
        span.getBytes(ref, offset, valuePA, actualSize) &&
        actualSize >= minSize)
        return valuePA;
    else
        return 0;
}

const uint32_t Elf::Program::getMetaSpanOffset(FlashBlockRef &ref,
        uint16_t key, uint32_t minSize, uint32_t &actualSize) const
{
    /*
     * Base accessor for metadata. Looks for 'key' within the metadata
     * segment. If the key exists, is properly aligned, and at least
     * 'minSize' bytes of data are valid, we return a byte offset to the
     * key within the program span.
     *
     * If any of these requirements are not met, returns 0.
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
     *
     * Note: Our program Header ref is only valid until we recycle 'ref'.
     * Use the program header to initialize lower and upper iteration bounds.
     */

    const uint32_t keySize = sizeof(_SYSMetadataKey);
    FlashMapSpan::ByteOffset I, E;
    {
        const ProgramHeader *ph = getMetadataSegment(ref);
        if (!ph)
            return 0;
        I = ph->p_offset;
        E = I + ph->p_filesz - keySize;
    }

    bool foundKey = false;
    uint32_t valueOffset = 0;

    while (I <= E) {
        uint32_t length = keySize;
        FlashMapSpan::PhysAddr recordPA;

        if (!span.getBytes(ref, I, recordPA, length) || length != keySize)
            return 0;
        const _SYSMetadataKey *record =
            reinterpret_cast<const _SYSMetadataKey *>(recordPA);
        I += keySize;

        bool isLast = record->stride >> 15;
        uint16_t stride = record->stride & 0x7FFF;

        if (record->key == key) {
            actualSize = stride;
            foundKey = true;
        } else if (!foundKey) {
            valueOffset += stride;
        }

        if (isLast) {
            // Key was missing?
            if (!foundKey)
                return 0;

            // Now we can calculate the address of the value, yay.
            return valueOffset + I;
        }
    }
    
    // Never found the "last key" marker :(
    return 0;
}
