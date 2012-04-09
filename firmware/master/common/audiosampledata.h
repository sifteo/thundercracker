/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef AUDIOSAMPLEDATA_H_
#define AUDIOSAMPLEDATA_H_

#include <stddef.h>
#include <sifteo/abi.h>
#include "macros.h"
#include "adpcmdecoder.h"

 #define LGPFX "AudioSampleData: "

class AudioSampleData {
public:
    AudioSampleData() : mod(0) {}

    void init(const struct _SYSAudioModule *module, uint32_t loop_start = 0) {
        mod = module;
        ringPos = 0;
        snapshotData.newestSample = 0;
        loopStart = loop_start;
        reset();
    }

    // Seek to the beginning of the sample data.
    void reset() {
        ASSERT(mod);
        newestSample = (uint32_t)-1;
        bufPos = 0;
        if (mod->type == _SYS_ADPCM) adpcmDec.reset();
    }

    inline uint32_t numSamples() const {
        if (mod == NULL) return 0;
        if (mod->type == _SYS_PCM) return mod->dataSize / sizeof(uint16_t);
        if (mod->type == _SYS_ADPCM) return mod->dataSize * kNibblesPerByte;
        ASSERT(mod->type == _SYS_PCM || mod->type == _SYS_ADPCM);
        return 0;
    }

    // Get a sample.
    int16_t operator[](uint32_t sampleNum);

    /* This should be called after a unit of work has been finished, to
     * recycle the FlashBlockRef.
     */
    void releaseRef() {
        ref.release();
    }

private:
    // Magic number for when the samples ringbuffer contains no valid values.
    static const uint32_t kNoSamples = (uint32_t)-1;

    // For code readability. You already know how many nibbles are in a byte.
    static const uint8_t kNibblesPerByte = 2;

    /* Buffer size should be at least 2, since callers are likely to request
     * neighbouring samples for interpolation. Since it's impractical to index
     * through samples backwards, there's also no reason for it to be any
     * larger.
     */
    static const uint8_t kSampleBufSize = 2;

    const struct _SYSAudioModule *mod; 

    FlashBlockRef ref;  // Released by caller when a unit of work is complete.

    uint16_t samples[kSampleBufSize];   // Mini-ringbuffer of samples
    uint32_t newestSample;              // Index of newest sample in samples[]
    uint8_t ringPos;                    // Points to the beginning of the ring
    ptrdiff_t bufPos;                   // Byte offset into sample data
    uint32_t loopStart;                 // Auto-snapshot sample index

    // ADPCM decoder state
    AdPcmDecoder adpcmDec;

    // Sample index of the oldest available sample
    inline uint32_t oldestSample() const {
        // Technically this is a lie.
        if (newestSample == kNoSamples) return 0;
        // But these aren't.
        if (newestSample < arraysize(samples)) return 0;
        return newestSample - arraysize(samples) + 1;
    }

    // Called from the decoder, write the next sample
    void writeNextSample(uint16_t sample);

    // Bytes taken up by $samples samples
    uint32_t bytesForSamples(uint32_t samples) const;

    // Decode up to and including the sampleNum sample
    void decodeToSample(uint32_t sampleNum);

    /* Snapshots:
     * Decoding to the beginning of an audio loop that's a significant number
     * of samples from the beginning of the file is memory and
     * processor-intensive. By opportunistically remembering the object's state
     * when the first sample of a loop is cached, it's possible to revert to
     * that state later with no performance penalty (and a small memory
     * penalty for storing the snapshot data).
     */
    struct {
        uint16_t samples[kSampleBufSize];
        uint32_t newestSample;
        uint8_t ringPos;
        ptrdiff_t bufPos;
        AdPcmDecoder adpcmDec;
    } snapshotData;

    void snapshot();
    bool hasSnapshot();
    void revert();
};

#endif // AUDIOSAMPLEDATA_H_
