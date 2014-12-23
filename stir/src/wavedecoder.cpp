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

#include "wavedecoder.h"
#include "script.h"

#include <errno.h>
#include <string.h>

namespace Stir {

bool WaveDecoder::loadFile(std::vector<unsigned char>& buffer, uint32_t &sampleRate, const std::string& filename, Stir::Logger &log)
{
    FILE *f = fopen(filename.c_str(), "rb");
    if (!f) {
        log.error("couldn't open wav file %s, %s\n", filename.c_str(), strerror(errno));
        return false;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        return false;
    }
    long filesz = ftell(f);
    rewind(f);

    /*
     * Make sure we're reading a legit RIFF file.
     */
    if (!readAndValidateRiff(log, f)) {
        return false;
    }

    /*
     * Now read through all sub chunks and process the ones we're interested in,
     * namely "fmt" and "data". There may be other subchunks interleaved.
     */

    bool foundFmt = false;
    bool foundData = false;

    while (ftell(f) < filesz) {
        ChunkHeader header;
        if (fread(&header, 1, sizeof header, f) != sizeof header) {
            log.error("i/o failure reading ChunkHeader\n");
            return false;
        }

        static const char fmt[4] = { 'f', 'm', 't', ' ' };
        static const char datachunk[4] = { 'd', 'a', 't', 'a' };

        if (memcmp(header.id, fmt, sizeof fmt) == 0) {
            FormatDescriptor fd;
            if (!readAndValidateFormat(header, fd, log, f)) {
                return false;
            }

            sampleRate = fd.sampleRate;
            foundFmt = true;

        } else if (memcmp(header.id, datachunk, sizeof datachunk) == 0) {
            foundData = readDataChunk(header, buffer, log, f);
            // if we already found format, we already have everything we need
            if (foundFmt) {
                break;
            }

        } else {
            // some other subchunk - skip it and keep looking
            if (fseek(f, header.size, SEEK_CUR) != 0) {
                log.error("i/o failure while seeking past chunk\n");
                return false;
            }
        }
    }

    if (!foundFmt) {
        log.error("didn't find a 'fmt' subchunk\n");
    }

    if (!foundData) {
        log.error("didn't find a 'data' subchunk\n");
    }

    return (foundFmt && foundData);
}

bool WaveDecoder::readAndValidateRiff(Logger &log, FILE *f)
{
    RiffDescriptor rd;
    if (fread(&rd, 1, sizeof rd, f) != sizeof rd) {
        log.error("RiffDescriptor: i/o failure\n");
        return false;
    }

    const char riff[4] = { 'R', 'I', 'F', 'F' };
    if (memcmp(rd.header.id, riff, sizeof riff)) {
        log.error("header didn't match (RIFF)\n");
        return false;
    }

    const char wave[4] = { 'W', 'A', 'V', 'E' };
    if (memcmp(rd.wave, wave, sizeof wave)) {
        log.error("header didn't match (WAVE)\n");
        return false;
    }

    return true;
}

bool WaveDecoder::readAndValidateFormat(const ChunkHeader &header, FormatDescriptor &fd, Stir::Logger &log, FILE *f)
{
    /*
     * Verify details of the format.
     * Right now we're a bit strict, but this has the positive side effect
     * of forcing people to prepare their content in a compatible way.
     */

    if (fread(&fd, 1, sizeof fd, f) != sizeof fd) {
        log.error("FormatDescriptor: i/o failure\n");
        return false;
    }

    if (fd.audioFormat != 0x1) {
        log.error("unknown audio format, we only accept PCM (0x1)\n");
        return false;
    }

    if (fd.numChannels != 1) {
        log.error("unsupported number of channels (%d), we only do mono\n", fd.numChannels);
        return false;
    }

    if (fd.sampleRate > Sound::STANDARD_SAMPLE_RATE) {
        log.error("high sample rate detected (%.1fkHz), %.1fkHz is max system rate",
                  fd.sampleRate / 1000.0, Sound::STANDARD_SAMPLE_RATE / 1000.0);
        return false;
    }

    // TODO: convert 8 to 16 bits, with a warning
    if (fd.bitsPerSample != 16) {
        log.error("unsupported bits per sample (%d), we only support 16-bit audio\n", fd.sampleRate);
        return false;
    }

    /*
     * Some encoders appear to include the ExtraParamSize field at the end
     * of FormatDescriptor, which should not exist for PCM format.
     *
     * Verify that at least in this case, ExtraParamSize is 0.
     */
    unsigned pos = ftell(f);
    unsigned offset = sizeof(RiffDescriptor) + sizeof(header) + header.size;
    if (offset > pos) {
        uint16_t extraParamSize;
        if (fread(&extraParamSize, 1, sizeof extraParamSize, f) != sizeof extraParamSize) {
            log.error("i/o error reading ExtraParamSize\n");
            return false;
        }

        if (extraParamSize != 0) {
            log.error("error: ExtraParamSize was non-zero for a PCM file\n");
            return false;
        }
    }

    return true;
}

bool WaveDecoder::readDataChunk(const ChunkHeader &header, std::vector<unsigned char>& buffer, Logger &log, FILE *f)
{
    buffer.resize(header.size);
    if (fread(&buffer[0], 1, header.size, f) != header.size) {
        log.error("i/o failure while reading data chunk\n");
        return false;
    }

    return true;
}

} // namespace Stir
