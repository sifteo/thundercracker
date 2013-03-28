/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "crc.h"
#include "hardware.h"
#include "sysinfo.h"


void Crc32::addUniqueness()
{
    const uint32_t* id = reinterpret_cast<const uint32_t*>(SysInfo::UniqueId);
    const unsigned numWords = SysInfo::UniqueIdNumBytes / sizeof(uint32_t);
        
    for (unsigned i = 0; i != numWords; ++i) {
        addInline(*id);
        id++;
    }
}

NEVER_INLINE void Crc32::add(uint32_t word)
{
    // Function call delay enforces minimum 4 AHB clock cycle delay.
    // This function should never be inlined!
    addInline(word);
}
