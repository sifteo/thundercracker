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
    void init(uint32_t loop_start = 0)
    {
        // Block boundary at which we take an automatic snapshot
        autoSnapshotPoint = loop_start & ~HALF_BUFFER_MASK;
        snapshot.sampleNum = 0x7fffffff & ~HALF_BUFFER_MASK;

        reset();
    }

    // Seek to the beginning of the sample data.
    void ALWAYS_INLINE reset()
    {
        state.sampleNum = 0;
        state.adpcm.init();
    }

    static uint32_t numSamples(const _SYSAudioModule &mod)
    {
        switch (mod.type) {    
            case _SYS_PCM:      return mod.dataSize / sizeof(uint16_t);
            case _SYS_ADPCM:    return mod.dataSize * NYBBLES_PER_BYTE;
            default:            return 0;
        }
    }

    /*
     * This should be called after a unit of work has been finished, to
     * recycle the FlashBlockRef.
     */
    void ALWAYS_INLINE releaseRef() {
        ref.release();
    }

    // Retrieve a single sample, via the cache
    int ALWAYS_INLINE getSample(unsigned sampleNum, const _SYSAudioModule &mod)
    {
        ASSERT(sampleNum < numSamples(mod));
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
        ASSERT(nextSample < numSamples(mod));
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

    FlashBlockRef ref;          // Released by caller when a unit of work is complete.
    uint32_t autoSnapshotPoint;

    struct State {
        uint32_t sampleNum;     // Half-buffer-aligned state from immediately before this sample #
        ADPCMState adpcm;       // ADPCM decoder state
    } state, snapshot;

    void fetchBlockPCM(uint32_t sampleNum, const _SYSAudioModule &mod);
    void fetchBlockADPCM(uint32_t sampleNum, const _SYSAudioModule &mod);

    void ALWAYS_INLINE fetchBlock(uint32_t sampleNum, const _SYSAudioModule &mod)
    {
        if (LIKELY(mod.type == _SYS_ADPCM))
            return fetchBlockADPCM(sampleNum, mod);

        ASSERT(mod.type == _SYS_PCM);
        return fetchBlockPCM(sampleNum, mod);
    }
};

#endif // AUDIOSAMPLEDATA_H_
