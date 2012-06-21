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
        // No snapshot yet
        snapshot.numSamples = 0;
    
        // numSamples at which we take an automatic snapshot
        loopPoint = loop_start ? (loop_start + kSampleBufSize) : 0;

        reset();
    }

    // Seek to the beginning of the sample data.
    void ALWAYS_INLINE reset()
    {
        state.numSamples = 0;
        state.bufPos = 0;
        state.adpcm.init();
    }

    static uint32_t numSamples(const _SYSAudioModule &mod)
    {
        switch (mod.type) {    
            case _SYS_PCM:      return mod.dataSize / sizeof(uint16_t);
            case _SYS_ADPCM:    return mod.dataSize * kNybblesPerByte;
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

        while (1) {
            // New sample
            if (sampleNum >= state.numSamples) {
                decodeToSample(sampleNum, mod);
                ASSERT(sampleNum + 1 == state.numSamples);
                break;
            }

            // Cached sample
            if (sampleNum + kSampleBufSize >= state.numSamples) {
                ASSERT(state.numSamples == sampleNum + 1 || state.numSamples == sampleNum + 2);
                break;
            }

            // Oops, we need to rewind. Do we have an applicable snapshot?
            // (NB: Rollover in the subtraction is okay, it will cause this to be false)
            // We may still have to roll forward from the snapshot.

            if (hasSnapshot() && sampleNum >= (snapshot.numSamples - kSampleBufSize)) {
                revertToSnapshot();
                continue;
            }

            // Back to the beginning (last resort)
            reset();
        }

        return state.samples[sampleNum % kSampleBufSize];
    }

    // Optimized accessor for a pair of samples (sampleNum, sampleNum + 1)
    int ALWAYS_INLINE getSamplePair(unsigned sampleNum, const _SYSAudioModule &mod, int &sample)
    {
        unsigned nextSampleNum = sampleNum + 1;
        ASSERT(nextSampleNum < numSamples(mod));

        while (1) {
            // At least one new sample
            if (nextSampleNum >= state.numSamples) {
                decodeToSample(nextSampleNum, mod);
                break;
            }

            // Both samples cached already
            if (nextSampleNum + 1 == state.numSamples) {
                break;
            }

            // Oops, we need to rewind. Do we have an applicable snapshot?
            // We may still need to roll forward from here.
            if (hasSnapshot() && sampleNum >= (snapshot.numSamples - kSampleBufSize)) {
                revertToSnapshot();
                continue;
            }

            // Last resort... Reset and retry.
            reset();
        }

        // We have the pair of samples in our buffer
        ASSERT(state.numSamples == sampleNum + 2);
        sample = state.samples[sampleNum % kSampleBufSize];
        return state.samples[nextSampleNum % kSampleBufSize];
    }

private:
    static const uint8_t kNybblesPerByte = 2;

    /*
     * Buffer size should be at least 2, since callers are likely to request
     * neighbouring samples for interpolation. Since it's impractical to index
     * through samples backwards, there's also no reason for it to be any
     * larger.
     */
    static const uint8_t kSampleBufSize = 2;

    FlashBlockRef ref;      // Released by caller when a unit of work is complete.
    uint32_t loopPoint;     // Auto-snapshot sample index

    struct State {
        int16_t samples[kSampleBufSize];    // Direct-mapped cache of samples
        uint32_t numSamples;                // 1 + index of last sample
        uint32_t bufPos;                    // Byte offset into sample data
        ADPCMState adpcm;                   // ADPCM decoder state
    } state, snapshot;

    // Decode up to and including the sampleNum sample
    void decodeToSamplePCM(uint32_t sampleNum, const _SYSAudioModule &mod);
    void decodeToSampleADPCM(uint32_t sampleNum, const _SYSAudioModule &mod);

    void ALWAYS_INLINE decodeToSample(uint32_t sampleNum, const _SYSAudioModule &mod)
    {
        if (LIKELY(mod.type == _SYS_ADPCM))
            return decodeToSampleADPCM(sampleNum, mod);

        ASSERT(mod.type == _SYS_PCM);
        return decodeToSamplePCM(sampleNum, mod);
    }

    /*
     * Snapshots:
     *
     * Decoding to the beginning of an audio loop that's a significant number
     * of samples from the beginning of the file is memory and
     * processor-intensive. By opportunistically remembering the object's state
     * when the first sample of a loop is cached, it's possible to revert to
     * that state later with no performance penalty (and a small memory
     * penalty for storing the snapshot data).
     */

    void ALWAYS_INLINE takeSnapshot() {
        snapshot = state;
    }

    void ALWAYS_INLINE revertToSnapshot() {
        state = snapshot;
    }

    bool ALWAYS_INLINE hasSnapshot() const {
        return snapshot.numSamples;
    }

};

#endif // AUDIOSAMPLEDATA_H_
