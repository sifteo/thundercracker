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

#ifndef SVM_FASTLZ_H
#define SVM_FASTLZ_H

#include "macros.h"
#include "svmmemory.h"


class SvmFastLZ {
public:

    /**
     * Decompress FastLZ Level 1 data from virtual memory.
     *
     * Decodes exactly 'srcLen' bytes of data from 'src'.
     * Writes at most 'destLen' bytes to 'dest'. Returns the number
     * of bytes actually decompressed via 'destLen'.
     *
     * On memory mapping failure or unexpected end-of-stream, returns
     * false. On success, returns true.
     *
     * Note that this doesn't support Level 2 data at all, nor does it expect
     * there to be a header to disambiguate beween Level 1 and Level 2 data.
     *
     * This function is guaranteed not to crash or write out of bounds,
     * even if the input data is corrupt or malicious.
     */
    static bool decompressL1(FlashBlockRef &ref, SvmMemory::PhysAddr dest,
        uint32_t &destLen, SvmMemory::VirtAddr src, uint32_t srcLen);

private:
    SvmFastLZ();    // Do not implement
};


#endif // SVM_FASTLZ_H
