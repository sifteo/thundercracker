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

#ifndef ELF_UTIL_H
#define ELF_UTIL_H

#include "macros.h"
#include "elfdefs.h"
#include "flash_blockcache.h"
#include "flash_map.h"
#include "flash_volume.h"

namespace Elf {


/**
 * Represents an ELF program, in flash. This provides validation for the
 * ELF headers, as well as accessors for loading header data out of flash.
 *
 * This stores very little in RAM: we try to avoid duplicating anything that
 * can be recreated from data in flash.
 */

class Program {
public:
    bool init(const FlashMapSpan &span);

    // Basic header accessors
    const FileHeader *getFileHeader(FlashBlockRef &ref) const;
    const ProgramHeader *getProgramHeader(FlashBlockRef &ref, unsigned index) const;
    const ProgramHeader *getProgramHeader(const FileHeader *fh, unsigned index) const;

    // Look up specific program headers
    const ProgramHeader *getMetadataSegment(FlashBlockRef &ref) const;
    const ProgramHeader *getRODataSegment(FlashBlockRef &ref) const;
    const ProgramHeader *getRWDataSegment(FlashBlockRef &ref) const;

    const uint32_t getTopOfRAM() const;
    uint32_t getEntry() const;

    const FlashMapSpan getProgramSpan() const {
        return span;
    }

    const FlashMapSpan getRODataSpan() const;

    // Return mapped metadata
    const char *getMetaString(FlashBlockRef &ref, uint16_t key) const;
    const void *getMeta(FlashBlockRef &ref, uint16_t key, uint32_t size) const;
    const void *getMeta(FlashBlockRef &ref, uint16_t key,
        uint32_t minSize, uint32_t &actualSize) const;

    static uint32_t copyMeta(const FlashVolume &vol, uint16_t key, uint32_t minSize,
                             uint32_t maxSize, uint8_t *buf);

    // Return unmapped metadata
    const uint32_t getMetaSpanOffset(FlashBlockRef &ref, uint16_t key,
        uint32_t minSize, uint32_t &actualSize) const;

    template <typename T>
    inline const T* getMetaValue(FlashBlockRef &ref, uint16_t key) const {
        return reinterpret_cast<const T*>(getMeta(ref, key, sizeof(T)));
    }

private:
    FlashMapSpan span;
};


}  // end namespace Elf

#endif // ELF_UTIL_H
