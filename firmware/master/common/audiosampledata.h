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
#include "flash_blockcache.h"


class AudioSampleData {
public:
    void init(const _SYSAudioModule &mod);

    // Seek to the beginning of the sample data.
    void ALWAYS_INLINE reset()
    {
        state.sampleNum = 0;
    }

    // Retrieve a single sample, via the cache
    int ALWAYS_INLINE getSample(unsigned sampleNum, const _SYSAudioModule &mod)
    {
        ASSERT(sampleNum < maxNumSamples(mod));
        ASSERT((state.sampleNum & HALF_BUFFER_MASK) == 0);

        unsigned diff = state.sampleNum - (sampleNum + 1);
        if (UNLIKELY(diff >= FULL_BUFFER))
            fetchBlock(sampleNum & ~HALF_BUFFER_MASK, mod);

        return samples[sampleNum & FULL_BUFFER_MASK];
    }

    // Optimized accessor for a pair of samples (sampleNum, sampleNum + 1)
    int ALWAYS_INLINE getSamplePair(unsigned sampleNum, const _SYSAudioModule &mod, int &sample)
    {
        unsigned nextSample = sampleNum + 1;
        ASSERT(nextSample < maxNumSamples(mod));
        ASSERT((state.sampleNum & HALF_BUFFER_MASK) == 0);

        unsigned diff = state.sampleNum - (nextSample + 1);
        if (UNLIKELY(diff >= FULL_BUFFER))
            fetchBlock(nextSample & ~HALF_BUFFER_MASK, mod);

        sample = samples[sampleNum & FULL_BUFFER_MASK];
        return samples[nextSample & FULL_BUFFER_MASK];
    }

private:
    static const unsigned NYBBLES_PER_BYTE = 2;

    static const unsigned FULL_BUFFER = 32;                 // Must be a power of two
    static const unsigned HALF_BUFFER = FULL_BUFFER / 2;
    static const unsigned FULL_BUFFER_MASK = FULL_BUFFER - 1;
    static const unsigned HALF_BUFFER_MASK = HALF_BUFFER - 1;

    int16_t samples[FULL_BUFFER];

    uint32_t autoSnapshotPoint;

    struct State {
        uint32_t sampleNum;     // Half-buffer-aligned state from immediately before this sample #
        ADPCMState adpcm;       // ADPCM decoder state
    } state, snapshot;

    ADPCMState adpcmIC;         // Initial conditions for ADPCM codec

    void fetchBlockPCM(uint32_t sampleNum, const _SYSAudioModule &mod);
    void fetchBlockADPCM(uint32_t sampleNum, const _SYSAudioModule &mod);

    /*
     * Grab the next block of audio samples, populating the cache block
     * starting at sampleNum.
     */
    void ALWAYS_INLINE fetchBlock(uint32_t sampleNum, const _SYSAudioModule &mod)
    {
        if (LIKELY(mod.type == _SYS_ADPCM))
            return fetchBlockADPCM(sampleNum, mod);

        ASSERT(mod.type == _SYS_PCM);
        return fetchBlockPCM(sampleNum, mod);
    }

    /*
     * At most, how many samples could this buffer hold?
     * Calculated based on the size of the module's compressed data buffer.
     *
     * This isn't exact, because of padding, headers, etc.
     * We shouldn't rely on this number to know when the
     * sample ends; instead, loopEnd must serve this purpose.
     */
    static uint32_t maxNumSamples(const _SYSAudioModule &mod)
    {
        switch (mod.type) {    
            case _SYS_PCM:      return mod.dataSize / sizeof(uint16_t);
            case _SYS_ADPCM:    return mod.dataSize * NYBBLES_PER_BYTE;
            default:            return 0;
        }
    }
};

#endif // AUDIOSAMPLEDATA_H_
