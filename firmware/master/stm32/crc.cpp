/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "crc.h"
#include "hardware.h"
#include "board.h"


void Crc32::addUniqueness()
{
    const uint32_t* id = reinterpret_cast<const uint32_t*>(Board::UniqueId);
    const unsigned numWords = Board::UniqueIdNumBytes / sizeof(uint32_t);
        
    for (unsigned i = 0; i != numWords; ++i) {
        add(*id);
        id++;
    }
}
