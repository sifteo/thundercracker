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


void CrcStream::addBytes(const uint8_t *bytes, uint32_t count)
{
    /*
     * Handle fully-aligned words, if any
     */

    if ((byteTotal & 3) == 0 && ((uintptr_t)bytes & 3) == 0) {
        unsigned words = count >> 2;
        byteTotal += words << 2;
        count &= 3;

        while (words) {
            words--;
            Crc32::add(*(const uint32_t*)bytes);
            bytes += 4;
        }
    }

    /*
     * Handle individual bytes
     */

    while (count) {
        buffer.bytes[byteTotal & 3] = *bytes;
        bytes++;
        byteTotal++;
        count--;

        if ((byteTotal & 3) == 0) {
            // Just completed a word
            Crc32::add(buffer.word);
        }
    }
}

void CrcStream::padToAlignment(unsigned alignment, uint8_t padByte)
{ 
    /*
     * Pad until alignment boundary
     */

    unsigned alignMask = alignment - 1;
    ASSERT(alignment >= 4);
    ASSERT((alignment & alignMask) == 0);

    while (byteTotal & alignMask) {
        buffer.bytes[byteTotal & 3] = 0xFF;
        byteTotal++;
        if ((byteTotal & 3) == 0) {
            // Just completed a word
            Crc32::add(buffer.word);
        }
    }

    ASSERT((byteTotal & 3) == 0);
}
