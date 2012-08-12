#ifndef WAVDECODER_H
#define WAVDECODER_H

#include "logger.h"

#include <stdint.h>
#include <vector>
#include <string>

class WaveDecoder {
public:
    WaveDecoder() {} // don't implement
    static bool loadFile(std::vector<unsigned char>& buffer, const std::string& filename, Stir::Logger &log);

private:
    struct RiffDescriptor {
        char riff[4];
        uint32_t chunkSize;
        char wave[4];
    };

    struct FormatDescriptor {
        struct {
            char subchunk1ID[4];
            uint32_t subchunk1Size;
        } header;
        uint16_t audioFormat;
        uint16_t numChannels;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample;
    };

    struct DataDescriptor {
        char subchunk2ID[4];
        uint32_t subchunk2Size;
    };
};

#endif // WAVDECODER_H
