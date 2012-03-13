/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef ELF_DEBUG_INFO_H
#define ELF_DEBUG_INFO_H

#include "elfdefs.h"
#include "flashlayer.h"

class ELFDebugInfo {
public:
    void init(const FlashRange &elf);
    
    bool readString(const char *section, uint32_t offset,
        char *buffer, uint32_t bufferSize);

    bool findNearestSymbol(uint32_t address, Elf::Symbol &symbol,
        char *name, uint32_t nameBufSize);

    void formatAddress(uint32_t address, char *buf, uint32_t bufSize);

private:
    struct SectionInfo {
        Elf::SectionHeader header;
        FlashRange data;
        char name[64];
    };

    static const unsigned MAX_DEBUG_SECTIONS = 32;
    SectionInfo sections[MAX_DEBUG_SECTIONS];

    const SectionInfo *findSection(const char *name) const;
};

#endif // ELF_DEBUG_INFO_H
