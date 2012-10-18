#include "wavedecoder.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

bool WaveDecoder::loadFile(std::vector<unsigned char>& buffer, const std::string& filename, Stir::Logger &log)
{
    FILE *f = fopen(filename.c_str(), "rb");
    if (!f) {
        log.error("couldn't open wav file %s, %s\n", filename.c_str(), strerror(errno));
        return false;
    }

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

    /*
     * Verify details of the format.
     * Right now we're a bit strict, but this has the positive side effect
     * of forcing people to prepare their content in a compatible way.
     */
    FormatDescriptor fd;
    if (fread(&fd, 1, sizeof fd, f) != sizeof fd) {
        log.error("FormatDescriptor: i/o failure\n");
        return false;
    }

    const char fmt[4] = { 'f', 'm', 't', ' ' };
    if (memcmp(fd.header.id, fmt, sizeof fmt)) {
        log.error("header didn't match (fmt)\n");
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

    if (fd.sampleRate != 16000) {
        log.error("unsupported sample rate (%d), we only support 16kHz\n", fd.sampleRate);
        return false;
    }

    // TODO: convert 8 to 16 bits, with a warning
    if (fd.bitsPerSample != 16) {
        log.error("unsupported bits per sample (%d), we only support 16-bit audio\n", fd.sampleRate);
        return false;
    }

    /*
     * Some encoders appear to include the ExtraParamSize field at the end
     * of FormatDescriptor that should not exist for PCM format.
     *
     * Verify that at least in this case, ExtraParamSize is 0.
     */
    unsigned pos = ftell(f);
    unsigned offset = sizeof(RiffDescriptor) + sizeof(fd.header) + fd.header.size;
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

    while (!feof(f)) {

        ChunkHeader sc;
        if (fread(&sc, 1, sizeof sc, f) != sizeof sc) {
            log.error("i/o failure reading ChunkHeader\n");
            return false;
        }

        static const char datachunk[4] = { 'd', 'a', 't', 'a' };

        if (memcmp(sc.id, datachunk, sizeof datachunk) == 0) {
            // found it!
            buffer.resize(sc.size);
            if (fread(&buffer.front(), 1, sc.size, f) != sc.size) {
                log.error("i/o failure while reading data chunk\n");
                return false;
            }

            return true;
        }

        // some other subchunk - skip it and keep looking
        if (fseek(f, sc.size, SEEK_CUR) != 0) {
            log.error("i/o failure while seeking past chunk\n");
            return false;
        }
    }

    log.error("didn't find a 'data' subchunk\n");
    return false;
}
