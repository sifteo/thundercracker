#ifndef WAVDECODER_H
#define WAVDECODER_H

#include "logger.h"

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
        ChunkHeader header;
        uint16_t audioFormat;
        uint16_t numChannels;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample;
    };


};

} // namespace Stir

#endif // WAVDECODER_H
