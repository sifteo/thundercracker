/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef ELF_DEBUG_INFO_H
#define ELF_DEBUG_INFO_H

#include "elfdefs.h"
#include "flashlayer.h"
#include <vector>
#include <map>
#include <string>

class ELFDebugInfo {
public:
    void init(const FlashRange &elf);

    std::string readString(const std::string &section, uint32_t offset);
    bool findNearestSymbol(uint32_t address, Elf::Symbol &symbol, std::string &name);
    std::string formatAddress(uint32_t address);

private:
    struct SectionInfo {
        Elf::SectionHeader header;
        FlashRange data;
    };

    typedef std::vector<SectionInfo> sections_t;
    typedef std::map<std::string, SectionInfo*> sectionMap_t;

    sections_t sections;
    sectionMap_t sectionMap;

    static void demangle(std::string &name);
    std::string readString(const SectionInfo *SI, uint32_t offset);
    const SectionInfo *findSection(const std::string &name);
};

#endif // ELF_DEBUG_INFO_H
