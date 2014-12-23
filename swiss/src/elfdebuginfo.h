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

#ifndef ELF_DEBUG_INFO_H
#define ELF_DEBUG_INFO_H

#include "elfdefs.h"
#include "mappedfile.h"

#include <vector>
#include <map>
#include <string>

class ELFDebugInfo {
public:
    void clear();
    bool init(const char *elfPath);

    std::string readString(const std::string &section, uint32_t offset) const;
    bool findNearestSymbol(uint32_t address, Elf::Symbol &symbol, std::string &name) const;
    std::string formatAddress(uint32_t address) const;
    bool readROM(uint32_t address, uint8_t *buffer, uint32_t bytes) const;

    bool metadataString(uint16_t key, std::string &s);
    uint8_t* metadata(uint16_t key, uint32_t &actualSize);

private:
    typedef std::vector<Elf::SectionHeader> sections_t;
    typedef std::map<std::string, Elf::SectionHeader*> sectionMap_t;

    MappedFile mappedFile;

    sections_t sections;
    sectionMap_t sectionMap;

    static void demangle(std::string &name);
    std::string readString(const Elf::SectionHeader *SI, uint32_t offset) const;

    const Elf::FileHeader *getFileHeader() const;
    const Elf::SectionHeader *findSection(const std::string &name) const;
    const Elf::ProgramHeader *getProgramHeader(const Elf::FileHeader *fh, unsigned index) const;

    const Elf::ProgramHeader *getMetadataSegment() const;

    bool copyProgramBytes(uint32_t byteOffset, uint8_t *dest, uint32_t length) const;
};

#endif // ELF_DEBUG_INFO_H
