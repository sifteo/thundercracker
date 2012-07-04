/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 *
 * Adapted from FastLZ, the lightning-fast lossless compression library.
 * This version has been pared down to only Level 1 decompression, and it's
 * been heavily modified to read its source data from SvmMemory.
 *
 * License for original FastLZ implementation (applies for this file only):
 *
 * Copyright (C) 2007 Ariya Hidayat (ariya@kde.org)
 * Copyright (C) 2006 Ariya Hidayat (ariya@kde.org)
 * Copyright (C) 2005 Ariya Hidayat (ariya@kde.org)
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

#include "svmfastlz.h"


/**
 * This is a small inlined utility class for safely and quickly reading a
 * byte stream from flash. Uses a tiny buffer to amortize costs (checking
 * boundary conditions, address translation) over many bytes.
 */
class LZByteReader {
public:

    ALWAYS_INLINE LZByteReader(FlashBlockRef &ref, SvmMemory::VirtAddr src, uint32_t srcLen)
        : ref(ref), src(src), srcLen(srcLen), bufLen(0)
    {}

    ALWAYS_INLINE ~LZByteReader() {}

    /// Are we at the end of the stream?
    ALWAYS_INLINE bool eof() const {
        return !(srcLen | bufLen);
    }

    /**
     * Read one byte from the stream, filling the buffer if needed.
     * If we're past the end of the stream or we hit a mapping error,
     * returns zero. eof() will be 'true' immediately after return in
     * this case.
     */
    ALWAYS_INLINE uint8_t read()
    {
        if (UNLIKELY(bufLen == 0)) {
            fillBuffer();
            if (UNLIKELY(bufLen == 0))
                return 0;
        }

        bufLen--;
        return *(bufPtr++);
    }

private:
    FlashBlockRef ref;
    SvmMemory::VirtAddr src;
    unsigned srcLen;            // Bytes remaining at 'src'
    unsigned bufLen;            // Bytes remaining in buffer
    uint8_t *bufPtr;            // Current read location in buffer
    uint8_t buffer[32];

    void fillBuffer();
};


void LZByteReader::fillBuffer()
{
    ASSERT(bufLen == 0);
    unsigned chunk = MIN(srcLen, sizeof buffer);
    if (!chunk)
        return;

    if (!SvmMemory::copyROData(ref, &buffer[0], src, chunk))
        return;

    bufLen = chunk;
    bufPtr = &buffer[0];
    src += chunk;
    srcLen -= chunk;
}


bool SvmFastLZ::decompressL1(FlashBlockRef &ref, SvmMemory::PhysAddr dest,
    uint32_t &destLen, SvmMemory::VirtAddr src, uint32_t srcLen)
{
    LZByteReader br(ref, src, srcLen);

    uint8_t *op = dest;
    uint8_t *op_limit = op + destLen;

    uint32_t ctrl = br.read() & 31;
    bool loop = true;

    do {
        const uint8_t* r = op;
        uint32_t len = ctrl >> 5;
        uint32_t ofs = (ctrl & 31) << 8;

        if (ctrl >= 32) {
            len--;
            r -= ofs;
            if (len == 7-1)
                len += br.read();
            r -= br.read();

            if (UNLIKELY(op + len + 3 > op_limit))
                return false;

            if (UNLIKELY(r-1 < (uint8_t *)dest))
                return false;

            if (UNLIKELY(br.eof())) 
                loop = 0;
            else
                ctrl = br.read();

            if (r == op) {
                /* optimize copy for a run */
                uint8_t b = r[-1];
                *op++ = b;
                *op++ = b;
                *op++ = b;
                for(; len; --len)
                    *op++ = b;
            } else {
                /* copy from rerence */
                r--;
                *op++ = *r++;
                *op++ = *r++;
                *op++ = *r++;
                for(; len; --len)
                    *op++ = *r++;
            }
        } else {
            ctrl++;

            if (UNLIKELY(op + ctrl > op_limit))
                return false;

            *op++ = br.read();
            for(--ctrl; ctrl; ctrl--)
                *op++ = br.read();

            loop = LIKELY(!br.eof());
            if (loop)
                ctrl = br.read();
        }
    } while (LIKELY(loop));

    ASSERT(unsigned(op - dest) <= destLen);
    destLen = op - dest;
    return true;
}
