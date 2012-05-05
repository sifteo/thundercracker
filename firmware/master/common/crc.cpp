/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Platform-independend CRC code
 */

#include "macros.h"
#include "crc.h"


uint32_t Crc32::block(const uint32_t *words, uint32_t count)
{
    reset();

    while (count) {
        add(*words);
        words++;
        count--;
    }

    return get();
}
