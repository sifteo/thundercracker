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
        log.error("i/o failure\n");
        return false;
    }

    const char riff[4] = { 'R', 'I', 'F', 'F' };
    if (memcmp(rd.riff, riff, sizeof riff)) {
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
        log.error("i/o failure\n");
        return false;
    }

    const char fmt[4] = { 'f', 'm', 't', ' ' };
    if (memcmp(fd.subchunk1ID, fmt, sizeof fmt)) {
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

    DataDescriptor dd;
    if (fread(&dd, 1, sizeof dd, f) != sizeof dd) {
        log.error("i/o failure\n");
        return false;
    }

    buffer.resize(dd.subchunk2Size);
    if (fread(&buffer.front(), 1, dd.subchunk2Size, f) != dd.subchunk2Size) {
        log.error("i/o failure\n");
        return false;
    }

    return true;
}
