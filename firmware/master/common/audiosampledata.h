/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef AUDIOSAMPLEDATA_H_
#define AUDIOSAMPLEDATA_H_

#include <sifteo/abi.h>
#include "macros.h"
#include "adpcmdecoder.h"

class AudioSampleData {
public:
    AudioSampleData() : mod(0) {}

    void init(const struct _SYSAudioModule *module) {
        mod = module;
        ringPos = 0;
        ref = NULL;
        reset();
    }

    void reset() {
        ASSERT(mod);
        newestSample = (uint32_t)-1;
        bufPos = 0;
    }

    inline uint32_t numSamples() const {
        if (mod == NULL) return 0;
        if (mod->type == _SYS_PCM) return mod->dataSize / sizeof(uint16_t);
        if (mod->type == _SYS_ADPCM) return mod->dataSize * kNibblesPerByte;
        ASSERT(mod->type == _SYS_PCM || mod->type == _SYS_ADPCM);
        return 0;
    }

    int16_t operator[](uint32_t sampleNum) {
        ASSERT(mod);
        ASSERT(sampleNum < numSamples());
        if (sampleNum < oldestSample() && newestSample != kNoSamples) reset();
        if (sampleNum > newestSample || newestSample == kNoSamples) decodeToSample(sampleNum);
        ASSERT(sampleNum <= newestSample && sampleNum >= oldestSample());
        return samples[((sampleNum - oldestSample()) + ringPos) % arraysize(samples)];
    }

    void useRef(FlashBlockRef *newRef) {
        ref = newRef;
    }

    void loseRef() {
        ref = NULL;
    }

private:
    const struct _SYSAudioModule *mod;
    FlashBlockRef *ref;
    uint16_t samples[2];   // mini-ringbuffer of samples
    uint32_t newestSample; // index of newest sample in samples[]
    uint8_t ringPos;       // points to the beginning of the ring
    uintptr_t bufPos;      // byte offset into sample data
    int debug;

    // Magic number for when the samples ringbuffer contains no valid values
    static const uint32_t kNoSamples = (uint32_t)-1;

    // For readability more than anything
    static const uint8_t kNibblesPerByte = 2;

    // ADPCM decoder state
    AdPcmDecoder adpcmDec;

    // sample index of the oldest available sample
    inline uint32_t oldestSample() const {
        // technically this is a lie.
        if (newestSample == kNoSamples) return 0;
        // but these aren't.
        if (newestSample < arraysize(samples)) return 0;
        return newestSample - arraysize(samples) + 1;
    }

    // Called from the decoder, write the next sample
    void writeNextSample(uint16_t sample) {
        samples[ringPos++] = sample;
        ringPos %= arraysize(samples);
        if(newestSample == kNoSamples) newestSample = 0;
        else newestSample++;
    }

    // Bytes taken up by $samples samples
    uint32_t bytesForSamples(uint32_t samples) const;

    // Decode until and including the sampleNum sample
    void decodeToSample(uint32_t sampleNum);
};

#endif // AUDIOSAMPLEDATA_H_
