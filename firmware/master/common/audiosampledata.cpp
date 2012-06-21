/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */
 
#include "audiosampledata.h"
#include "svmmemory.h"
#include <algorithm>

#define LGPFX "AudioSampleData: "


void AudioSampleData::decodeToSamplePCM(uint32_t sampleNum, const _SYSAudioModule &mod)
{
    /*
     * PCM provides random access, so all we need to do is update the
     * buffer position and copy up to two samples from virtual memory.
     */

    // Must be decoding at least one sample
    ASSERT(sampleNum >= state.numSamples);

    // How many additional samples, prior to the last one?
    unsigned addlCount = sampleNum - state.numSamples;
    state.numSamples = sampleNum + 1;

    if (LIKELY(addlCount == 0)) {
        // Fast path for single samples

        unsigned bufPos = state.bufPos;
        state.bufPos = bufPos + sizeof(int16_t);
        SvmMemory::VirtAddr va = mod.pData + bufPos;
        SvmMemory::PhysAddr pa = (SvmMemory::PhysAddr) &state.samples[sampleNum % kSampleBufSize];

        SvmMemory::copyROData(ref, pa, va, sizeof state.samples[0]);
        return;
    }

    /*
     * Otherwise, we're copying two samples. Depending on the final
     * value of numSamples, they may or may not be swapped in our buffer.
     */

    unsigned count = addlCount + 1;
    unsigned byteCount = count * sizeof(int16_t);
    SvmMemory::VirtAddr va = mod.pData + (state.bufPos += byteCount) - sizeof state.samples;

    if (sampleNum % kSampleBufSize) {
        // Last sample is at [1]. Samples are not swapped, we can copy right in.

        SvmMemory::PhysAddr pa = (SvmMemory::PhysAddr) &state.samples[0];
        SvmMemory::copyROData(ref, pa, va, sizeof state.samples);

    } else {
        // Last sample is at [0]. Copy two samples and exchange them.

        int16_t buffer[kSampleBufSize];
        SvmMemory::PhysAddr pa = (SvmMemory::PhysAddr) &buffer[0];
        SvmMemory::copyROData(ref, pa, va, sizeof buffer);
        state.samples[0] = buffer[1];
        state.samples[1] = buffer[0];
    }
}

void AudioSampleData::decodeToSampleADPCM(uint32_t sampleNum, const _SYSAudioModule &mod)
{
    // Fast local copy of ADPCM CODEC state
    ADPCMDecoder dec;
    dec.load(state.adpcm);

    // Local copies of other members
    SvmMemory::VirtAddr vaBase = mod.pData;
    int16_t *samples = state.samples;
    unsigned bufPos = state.bufPos;
    unsigned numSamples = state.numSamples;
    const unsigned latchedLoopPoint = loopPoint;
    const unsigned target = sampleNum + 1;
    ASSERT(target != numSamples);

    while (1) {
        SvmMemory::PhysAddr pa;
        uint32_t byteCount = ceildiv<unsigned>(target - numSamples, kNybblesPerByte);
        bool mapStatus = SvmMemory::mapROData(ref, vaBase + bufPos, byteCount, pa);

        if (UNLIKELY(!mapStatus)) {
            LOG((LGPFX "Memory mapping failure for ADPCM sample at VA 0x%08x+%x\n",
                unsigned(vaBase), bufPos));
            state.numSamples = target;
            return;
        }

        const uint8_t *ptr = pa;
        const uint8_t *limit = pa + byteCount;

        /*
         * Decode and store each sample. Technically we may not have to
         * store all of these samples, since only up to the last
         * kSampleBufSize samples are actually retained. But it's highly
         * uncommon to skip more than a single sample at a time, so instead
         * it makes more sense to optimize this loop for lightweight
         * control flow.
         */

        do {
            samples[(numSamples++) % kSampleBufSize] = dec.decodeSample(ptr);

            if (UNLIKELY(numSamples == latchedLoopPoint) && !hasSnapshot()) {
                state.bufPos = bufPos + (ptr - pa);
                state.numSamples = numSamples;
                dec.store(state.adpcm);
                takeSnapshot();
            }

            if (target == numSamples) {
                state.bufPos = bufPos + (ptr - pa);
                state.numSamples = numSamples;
                dec.store(state.adpcm);
                return;
            }

        } while (ptr != limit);
        bufPos += (ptr - pa);
    }
}
