/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
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

/*
 * Platform-independent CRC code
 */

#include "macros.h"
#include "crc.h"


uint32_t Crc32::block(const uint32_t *words, uint32_t count)
{
    reset();

    while (count) {
        addInline(*words);
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
            Crc32::addInline(*(const uint32_t*)bytes);
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
            Crc32::addInline(buffer.word);
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
            Crc32::addInline(buffer.word);
        }
    }

    ASSERT((byteTotal & 3) == 0);
}
