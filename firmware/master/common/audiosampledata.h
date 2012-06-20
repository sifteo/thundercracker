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
    AudioSampleData() : mod(0) {}

    void init(const struct _SYSAudioModule *module, uint32_t loop_start = 0);
    void reset();
    uint32_t numSamples() const;

    // Get a sample.
    int16_t operator[](uint32_t sampleNum);

    /*
     * This should be called after a unit of work has been finished, to
     * recycle the FlashBlockRef.
     */
    void releaseRef() {
        ref.release();
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

    const struct _SYSAudioModule *mod; 

    FlashBlockRef ref;      // Released by caller when a unit of work is complete.
    uint32_t loopPoint;     // Auto-snapshot sample index

    struct State {
        uint16_t samples[kSampleBufSize];   // Direct-mapped cache of samples
        uint32_t numSamples;                // 1 + index of last sample
        uint32_t bufPos;                    // Byte offset into sample data
        AdPcmDecoder adpcm;                 // ADPCM decoder state
    } state, snapshot;

    // Called from the decoder, write the next sample
    void writeNextSample(uint16_t sample);

    // Decode up to and including the sampleNum sample
    void decodeToSamplePCM(uint32_t sampleNum);
    void decodeToSampleADPCM(uint32_t sampleNum);
    void decodeToSampleSilence(uint32_t sampleNum);

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
