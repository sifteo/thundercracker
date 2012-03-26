/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <cxxabi.h>
#include "elfdebuginfo.h"
#include "flash.h"
#include <string.h>
#include <stdlib.h>

void ELFDebugInfo::init(const FlashRange &elf)
{
    /*
     * The ELF header has already been validated for runtime purposes
     * by ElfUtil. Now we want to pull out the section header table,
     * so that we can read debug symbols at runtime.
     */

    sections.clear();
    sectionMap.clear();

    FlashBlockRef ref;
    FlashBlock::get(ref, elf.getAddress());
    Elf::FileHeader *header = reinterpret_cast<Elf::FileHeader*>(ref->getData());

    // First pass, copy out all the headers.
    for (unsigned i = 0; i < header->e_shnum; i++) {
        sections.push_back(SectionInfo());
        SectionInfo &SI = sections.back();
        Elf::SectionHeader *pHdr = &SI.header;
        FlashRange rHdr = elf.split(header->e_shoff + i * header->e_shentsize, sizeof *pHdr);
        ASSERT(rHdr.getSize() == sizeof *pHdr);
        Flash::read(rHdr.getAddress(), (uint8_t*)pHdr, rHdr.getSize());
        SI.data = elf.split(pHdr->sh_offset, pHdr->sh_size);
    }

    // Read section names, set up sectionMap.
    if (header->e_shstrndx < sections.size()) {
        const SectionInfo *strTab = &sections[header->e_shstrndx];
        for (unsigned i = 0, e = sections.size(); i != e; ++i) {
            SectionInfo &SI = sections[i];
            sectionMap[readString(strTab, SI.header.sh_name)] = &SI;
        }
    }
}

const ELFDebugInfo::SectionInfo *ELFDebugInfo::findSection(const std::string &name) const
{
    sectionMap_t::const_iterator I = sectionMap.find(name);
    if (I == sectionMap.end())
        return 0;
    else
        return I->second;
}

std::string ELFDebugInfo::readString(const SectionInfo *SI, uint32_t offset) const
{
    char buf[1024];
    FlashRange r = SI->data.split(offset, sizeof buf - 1);
    Flash::read(r.getAddress(), (uint8_t*)buf, r.getSize());
    buf[r.getSize()] = '\0';
    return buf;
}

std::string ELFDebugInfo::readString(const std::string &section, uint32_t offset) const
{
    const SectionInfo *SI = findSection(section);
    if (SI)
        return readString(SI, offset);
    else
        return "";
}

bool ELFDebugInfo::findNearestSymbol(uint32_t address,
    Elf::Symbol &symbol, std::string &name) const
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

    if (bestOffset == worstOffset) {
        memset(&symbol, 0, sizeof symbol);
        name = "(unknown)";
        return false;
    }

    name = readString(".strtab", symbol.st_name);
    return true;
}

std::string ELFDebugInfo::formatAddress(uint32_t address) const
{
    Elf::Symbol symbol;
    std::string name;

    findNearestSymbol(address, symbol, name);
    demangle(name);
    uint32_t offset = address - symbol.st_value;

    if (offset != 0) {
        char buf[16];
        snprintf(buf, sizeof buf, "+0x%x", offset);
        name += buf;
    }

    return name;
}

void ELFDebugInfo::demangle(std::string &name)
{
    // This uses the demangler built into GCC's libstdc++.
    // It uses the same name mangling style as clang.

    int status;
    char *result = abi::__cxa_demangle(name.c_str(), 0, 0, &status);
    if (status == 0) {
        name = result;
        free(result);
    }
}

bool ELFDebugInfo::readROM(uint32_t address, uint8_t *buffer, uint32_t bytes) const
{
    /*
     * Try to read from read-only data in the ELF file. If the read can be
     * satisfied locally, from the ELF data, returns true and fills in 'buffer'.
     * If not, returns false.
     */

    for (sections_t::const_iterator I = sections.begin(), E = sections.end(); I != E; ++I) {

        // Section must be allocated program data
        if (I->header.sh_type != Elf::SHT_PROGBITS)
            continue;
        if (!(I->header.sh_flags & Elf::SHF_ALLOC))
            continue;

        // Section must not be writeable
        if (I->header.sh_flags & Elf::SHF_WRITE)
            continue;

        // Address range must be fully contained within the section
        uint32_t offset = address - I->header.sh_addr;
        FlashRange r = I->data.split(offset, bytes);
        if (r.getSize() != bytes)
            continue;

        // Success, we can read from the ELF
        Flash::read(r.getAddress(), buffer, bytes);
        return true;
    }

    return false;
}
