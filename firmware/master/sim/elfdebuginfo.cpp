/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <string.h>
#include "elfdebuginfo.h"
#include "flash.h"


const ELFDebugInfo::SectionInfo *ELFDebugInfo::findSection(const char *name) const
{
    for (unsigned i = 0; i < MAX_DEBUG_SECTIONS; i++) {
        const SectionInfo *SI = &sections[i];
        if (!strcmp(SI->name, name))
            return SI;
    }
    return 0;
}

void ELFDebugInfo::init(const FlashRange &elf)
{
    /*
     * The ELF header has already been validated for runtime purposes
     * by ElfUtil. Now we want to pull out the section header table,
     * so that we can read debug symbols at runtime.
     */

    memset(sections, 0, sizeof sections);

    FlashBlockRef ref;
    FlashBlock::get(ref, elf.getAddress());
    Elf::FileHeader *header = reinterpret_cast<Elf::FileHeader*>(ref->getData());

    // First pass, copy out all the headers.
    for (unsigned i = 0; i < header->e_shnum && i < MAX_DEBUG_SECTIONS; i++) {
        SectionInfo &SI = sections[i];
        Elf::SectionHeader *pHdr = &SI.header;
        FlashRange rHdr = elf.split(header->e_shoff + i * header->e_shentsize, sizeof *pHdr);
        ASSERT(rHdr.getSize() == sizeof *pHdr);
        Flash::read(rHdr.getAddress(), (uint8_t*)pHdr, rHdr.getSize());
        SI.data = elf.split(pHdr->sh_offset, pHdr->sh_size);
    }

    // Assign section names
    if (header->e_shstrndx < header->e_shnum && header->e_shstrndx < MAX_DEBUG_SECTIONS) {
        FlashRange strTab = sections[header->e_shstrndx].data;
        for (unsigned i = 0; i < header->e_shnum && i < MAX_DEBUG_SECTIONS; i++) {
            SectionInfo &SI = sections[i];
            FlashRange rStr = strTab.split(SI.header.sh_name, sizeof SI.name);
            Flash::read(rStr.getAddress(), (uint8_t*)SI.name, rStr.getSize());
            SI.name[sizeof SI.name - 1] = 0;
        }
    }
}

bool ELFDebugInfo::readString(const char *section, uint32_t offset,
    char *buffer, uint32_t bufferSize)
{
    const SectionInfo *SI = findSection(section);
    if (SI) {
        FlashRange r = SI->data.split(offset, bufferSize - 1);
        Flash::read(r.getAddress(), (uint8_t*)buffer, r.getSize());
        buffer[bufferSize - 1] = '\0';
        return true;
    }
    return false;
}

bool ELFDebugInfo::findNearestSymbol(uint32_t address, Elf::Symbol &symbol,
    char *name, uint32_t nameBufSize)
{
    // Look for the nearest ELF symbol which contains 'address'.
    // If we find anything, returns 'true' and leaves the symbol information
    // in "symbol", and the name is read into the "name" buffer.
    //
    // If nothing is found, we still fill in the output buffer and name
    // with "(unknown)" and zeroes, but 'false' is returned.

    const SectionInfo *SI = findSection(".symtab");
    Elf::Symbol currentSym;
    const uint32_t worstOffset = (uint32_t) -1;
    uint32_t bestOffset = worstOffset;

    for (unsigned index = 0;; index++) {
        FlashRange r = SI->data.split(index * sizeof currentSym, sizeof currentSym);
        if (r.getSize() != sizeof currentSym)
            break;
        Flash::read(r.getAddress(), (uint8_t*) &currentSym, r.getSize());

        uint32_t offset = address - currentSym.st_value;
        if (offset < currentSym.st_size && offset < bestOffset) {
            symbol = currentSym;
            bestOffset = offset;
        }
    }

    // Strip the Thumb bit from function symbols.
    if ((symbol.st_info & 0xF) == Elf::STT_FUNC)
        symbol.st_value &= ~1;

    strncpy(name, "(unknown)", nameBufSize);
    if (bestOffset == worstOffset) {
        memset(&symbol, 0, sizeof symbol);
        return false;
    }

    return readString(".strtab", symbol.st_name, name, nameBufSize);
}

void ELFDebugInfo::formatAddress(uint32_t address, char *buf, uint32_t bufSize)
{
    Elf::Symbol symbol;

    findNearestSymbol(address, symbol, buf, bufSize);
    uint32_t nameLen = strlen(buf);
    uint32_t offset = address - symbol.st_value;

    if (offset != 0)
        snprintf(buf + nameLen, bufSize - nameLen, "+0x%x", offset);
}
