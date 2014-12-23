/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * swiss - your Sifteo utility knife
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

#ifndef LFSVOLUME_H
#define LFSVOLUME_H

#include "savedata.h"
#include <vector>

/*
 * Stepping lightly with these includes, so as to avoid pulling in any
 * simulator dependencies...
 */
#include "flash_volumeheader.h"


/*
 * As found in firmware/master/common/flash_lfs.h
 * Duplicated in our own namespace here to avoid dependecy on emulator sources.
 */

namespace SwissLFS {
    const unsigned KEY_ANY = unsigned(-1);

    // Object sizes are 8-bit, measured in multiples of SIZE_UNIT
    const unsigned SIZE_SHIFT  = 4;

    uint8_t computeCheckByte(uint8_t a, uint8_t b);
    bool isEmpty(const uint8_t *bytes, unsigned count);
}

/*
 * A somewhat bastardized version of the one in firmware/master/common/flash_lfs.h
 */

struct FlashLFSIndexAnchor
{
    static const unsigned OFFSET_SHIFT = SwissLFS::SIZE_SHIFT;

    uint8_t offsetLow;
    uint8_t offsetHigh;
    uint8_t check;

    FlashLFSIndexAnchor(uint8_t *bytes)
    {
        offsetLow   = bytes[0];
        offsetHigh  = bytes[1];
        check       = bytes[2];
    }

    bool isValid() const {
        return check == SwissLFS::computeCheckByte(offsetLow, offsetHigh);
    }

    unsigned offsetInBytes() const {
        return (offsetLow | (offsetHigh << 8)) << OFFSET_SHIFT;
    }

    bool isEmpty() const {
        return offsetLow == 0xff && offsetHigh == 0xff && check == 0xff;
    }
};

/*
 * Similarly bastardized, original in firmware/master/common/flash_lfs.h
 */

struct FlashLFSIndexRecord
{
    static const unsigned SIZE_SHIFT = SwissLFS::SIZE_SHIFT;

    uint8_t key;
    uint8_t size;
    uint8_t crc[2];     // Unaligned on purpose...
    uint8_t check;      //   to keep the check byte at the end.

    FlashLFSIndexRecord(uint8_t *bytes)
    {
        key     = bytes[0];
        size    = bytes[1];
        crc[0]  = bytes[2];
        crc[1]  = bytes[3];
        check   = bytes[4];
    }

    bool isValid() const {
        return check == SwissLFS::computeCheckByte(key, size);
    }

    bool isEmpty() const {
        return  key     == 0xff &&
                size    == 0xff &&
                crc[0]  == 0xff &&
                crc[1]  == 0xff &&
                check   == 0xff;
    }

    unsigned sizeInBytes() const {
        return size << SIZE_SHIFT;
    }

    uint16_t crc16() const {
        return crc[1] << 8 | crc[0];
    }
};

class LFSVolume
{
public:
    LFSVolume(unsigned cacheBlockSize, unsigned blockSize);

    bool init(FILE *f);
    void retrieveRecords(SaveData::Records & records);

private:
    const unsigned CacheBlockSize;
    const unsigned BlockSize;

    FlashVolumeHeader header;
    std::vector<uint8_t> payload;

    unsigned objectDataIndex;

    bool retrieveRecordsFromBlock(SaveData::Records & records, std::vector<uint8_t> &indexBlock);
};

#endif // LFSVOLUME_H
