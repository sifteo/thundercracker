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
        log.error("FormatDescriptor: i/o failure\n");
        return false;
    }

    const char fmt[4] = { 'f', 'm', 't', ' ' };
    if (memcmp(fd.header.subchunk1ID, fmt, sizeof fmt)) {
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
     * Give them a chance, and seek ahead as specified by FormatDescriptor::subchunk1Size
     */
    unsigned pos = ftell(f);
    unsigned offset = sizeof(RiffDescriptor) + sizeof(fd.header) + fd.header.subchunk1Size;
    if (offset > pos) {
        if (offset - pos != sizeof(uint16_t)) {
            log.error("tried to handle oddly encoded file whose FormatDescriptor was larger"
                      "than expected, but it was just...too...large...\n");
            return false;
        }
        fseek(f, offset - pos, SEEK_CUR);
    }

    DataDescriptor dd;
    if (fread(&dd, 1, sizeof dd, f) != sizeof dd) {
        log.error("DataDescriptor: i/o failure\n");
        return false;
    }

    const char datachunk[4] = { 'd', 'a', 't', 'a' };
    if (memcmp(dd.subchunk2ID, datachunk, sizeof datachunk)) {
        log.error("header didn't match (data)\n");
        return false;
    }

    buffer.resize(dd.subchunk2Size);
    if (fread(&buffer.front(), 1, dd.subchunk2Size, f) != dd.subchunk2Size) {
        log.error("Read: i/o failure\n");
        return false;
    }

    return true;
}
