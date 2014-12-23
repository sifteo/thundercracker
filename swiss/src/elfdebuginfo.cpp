/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * swiss - your Sifteo utility knife
 *
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

#include "elfdebuginfo.h"
#include <cxxabi.h>
#include "macros.h"

#include <sifteo/abi/elf.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


/*
 * NOTE: at the moment, this is a slightly modified version of
 * emulator/src/mc_elfdebuginfo.cpp Eventually want to migrate that out of
 * the emulator, and make this available as library code to be shared by
 * both swiss and the emulator (if both end up wanting to use it).
 */

void ELFDebugInfo::clear()
{
    mappedFile.unmap();
    sections.clear();
    sectionMap.clear();
}

bool ELFDebugInfo::copyProgramBytes(uint32_t byteOffset, uint8_t *dest, uint32_t length) const
{
    /*
     * Copy data out of the program's ELF
     */
    if (!mappedFile.isMapped())
        return false;

    unsigned avail;
    uint8_t *p = mappedFile.getData(byteOffset, avail);
    if (!p || length > avail)
        return false;

    memcpy(dest, p, length);

    return true;
}

bool ELFDebugInfo::init(const char *elfPath)
{
    /*
     * Build a table of debug sections.
     */

    clear();
    if (!mappedFile.map(elfPath))
        return false;

    const Elf::FileHeader *header = getFileHeader();
    if (!header)
        return false;

    // First pass, copy out all the headers.
    for (unsigned i = 0; i < header->e_shnum; i++) {
        sections.push_back(Elf::SectionHeader());
        Elf::SectionHeader *pHdr = &sections.back();
        uint32_t off = header->e_shoff + i * header->e_shentsize;
        if (!copyProgramBytes(off, (uint8_t*)pHdr, sizeof *pHdr))
            memset(pHdr, 0xFF, sizeof *pHdr);
    }

    // Read section names, set up sectionMap.
    if (header->e_shstrndx < sections.size()) {
        const Elf::SectionHeader *strTab = &sections[header->e_shstrndx];
        for (unsigned i = 0, e = sections.size(); i != e; ++i) {
            Elf::SectionHeader *pHdr = &sections[i];
            sectionMap[readString(strTab, pHdr->sh_name)] = pHdr;
        }
    }

    return true;
}

/*
 * FileHeader is located at offset 0, and maps the rest of the ELF.
 */
const Elf::FileHeader *ELFDebugInfo::getFileHeader() const
{
    unsigned avail;
    uint8_t *fh = mappedFile.getData(0, avail);
    if (!fh || avail < sizeof(Elf::FileHeader))
        return 0;

    return reinterpret_cast<Elf::FileHeader*>(fh);
}

/*
 * We already collected a map of sections during init. Return the requested
 * entry if it exists.
 */
const Elf::SectionHeader *ELFDebugInfo::findSection(const std::string &name) const
{
    sectionMap_t::const_iterator I = sectionMap.find(name);
    if (I == sectionMap.end())
        return 0;
    else
        return I->second;
}

/*
 * Retrieve the requested ProgramHeader from the FileHeader, if available.
 */
const Elf::ProgramHeader *ELFDebugInfo::getProgramHeader(const Elf::FileHeader *fh, unsigned index) const
{
    ASSERT(index < fh->e_phnum);
    unsigned offset = fh->e_phoff + index * sizeof(Elf::ProgramHeader);

    unsigned avail;
    uint8_t *p = mappedFile.getData(offset, avail);
    if (!p || offset + sizeof(Elf::ProgramHeader) > avail)
        return 0;

    return reinterpret_cast<Elf::ProgramHeader*>((uint8_t*)fh + offset);
}

const Elf::ProgramHeader *ELFDebugInfo::getMetadataSegment() const
{
    const Elf::FileHeader *fh = getFileHeader();
    ASSERT(fh);

    for (unsigned i = 0; i < fh->e_phnum; ++i) {
        const Elf::ProgramHeader *ph = getProgramHeader(fh, i);
        if (!ph)
            break;

        // Segment must be of the proper type, and 32-bit aligned.
        if (ph->p_type == _SYS_ELF_PT_METADATA && (ph->p_offset & 3) == 0)
            return ph;
    }
    return 0;
}

std::string ELFDebugInfo::readString(const Elf::SectionHeader *SI, uint32_t offset) const
{
    static char buf[1024];
    uint32_t size = std::min<uint32_t>(sizeof buf - 1, SI->sh_size - offset);
    if (!copyProgramBytes(SI->sh_offset + offset, (uint8_t*)buf, size))
        size = 0;
    buf[size] = '\0';
    return buf;
}

std::string ELFDebugInfo::readString(const std::string &section, uint32_t offset) const
{
    const Elf::SectionHeader *SI = findSection(section);
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

    const Elf::SectionHeader *SI = findSection(".symtab");
    if (SI) {
        Elf::Symbol currentSym;
        const uint32_t worstOffset = (uint32_t) -1;
        uint32_t bestOffset = worstOffset;

        for (unsigned index = 0;; index++) {
            uint32_t tableOffset = index * sizeof currentSym;

            if (tableOffset + sizeof currentSym > SI->sh_size ||
                !copyProgramBytes(
                    SI->sh_offset + tableOffset,
                    (uint8_t*) &currentSym, sizeof currentSym))
                break;

            // Strip the Thumb bit from function symbols.
            if ((currentSym.st_info & 0xF) == Elf::STT_FUNC)
                currentSym.st_value &= ~1;

            uint32_t addrOffset = address - currentSym.st_value;
            if (addrOffset < currentSym.st_size && addrOffset < bestOffset) {
                symbol = currentSym;
                bestOffset = addrOffset;
            }
        }

        if (bestOffset != worstOffset) {
            name = readString(".strtab", symbol.st_name);
            return true;
        }
    }

    memset(&symbol, 0, sizeof symbol);
    name = "(unknown)";
    return false;
}


bool ELFDebugInfo::metadataString(uint16_t key, std::string &s)
{
    uint32_t actualSize;
    uint8_t *m = metadata(key, actualSize);
    if (!m)
        return false;

    s = std::string((const char*)m, actualSize);
    return true;
}

uint8_t* ELFDebugInfo::metadata(uint16_t key, uint32_t &actualSize)
{
    const Elf::ProgramHeader *ph = getMetadataSegment();
    if (!ph)
        return 0;

    const uint32_t keySize = sizeof(_SYSMetadataKey);
    unsigned I, E;
    I = ph->p_offset;
    E = I + ph->p_filesz - keySize;

    bool foundKey = false;
    uint32_t valueOffset = 0;
    unsigned avail;

    while (I <= E) {

        uint8_t *p = mappedFile.getData(I, avail);
        if (!p)
            return 0;

        const _SYSMetadataKey *record = reinterpret_cast<const _SYSMetadataKey *>(p);
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
            return mappedFile.getData(valueOffset + I, avail);
        }
    }

    // Ran past the end of the section
    return 0;
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
        if (I->sh_type != Elf::SHT_PROGBITS)
            continue;
        if (!(I->sh_flags & Elf::SHF_ALLOC))
            continue;

        // Section must not be writeable
        if (I->sh_flags & Elf::SHF_WRITE)
            continue;

        // Address range must be fully contained within the section
        uint32_t offset = address - I->sh_addr;
        if (offset > I->sh_size || offset + bytes > I->sh_size)
            continue;

        // Success, we can read from the ELF
        return copyProgramBytes(offset, buffer, bytes);
    }

    return false;
}
