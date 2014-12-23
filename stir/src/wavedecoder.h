/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
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

#ifndef WAVDECODER_H
#define WAVDECODER_H

#include "logger.h"

#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>

namespace Stir {

class WaveDecoder {
public:
    WaveDecoder() {} // don't implement
    static bool loadFile(std::vector<unsigned char>& buffer, uint32_t &sampleRate, const std::string& filename, Stir::Logger &log);

private:

    struct ChunkHeader {
        char id[4];
        uint32_t size;
    };

    struct RiffDescriptor {
        ChunkHeader header;
        char wave[4];
    };

    struct FormatDescriptor {
        uint16_t audioFormat;
        uint16_t numChannels;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample;
    };

    static bool readAndValidateRiff(Stir::Logger &log, FILE *f);
    static bool readAndValidateFormat(const ChunkHeader &header, FormatDescriptor &fd, Stir::Logger &log, FILE *f);
    static bool readDataChunk(const ChunkHeader &header, std::vector<unsigned char>& buffer, Stir::Logger &log, FILE *f);
};

} // namespace Stir

#endif // WAVDECODER_H
