/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
