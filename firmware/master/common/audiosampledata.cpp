/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */
 
#include "audiosampledata.h"
#include "svmmemory.h"

#define LGPFX "AudioSampleData: "


void AudioSampleData::init(const struct _SYSAudioModule *module, uint32_t loop_start)
{
    ASSERT(module);
    mod = module;
    snapshot.numSamples = 0;
    
    // numSamples at which we take an automatic snapshot
    loopPoint = loop_start ? (loop_start + kSampleBufSize) : 0;

    reset();
}

// Seek to the beginning of the sample data.
void AudioSampleData::reset()
{
    ASSERT(mod);
    state.numSamples = 0;
    state.bufPos = 0;

    if (mod->type == _SYS_ADPCM)
        state.adpcm.reset();
}

uint32_t AudioSampleData::numSamples() const
{
    ASSERT(mod);
    switch (mod->type) {
    
    case _SYS_PCM:
        return mod->dataSize / sizeof(uint16_t);

    case _SYS_ADPCM:
        return mod->dataSize * kNybblesPerByte;

    default:
        ASSERT(0);
        return 0;
    }
}

/*
 * Providing an array interface to sample data is convenient for the audio
 * channel, but it's a bit misleading because truly random access patterns
 * can incur massive performance penalties. In general, requests should be
 * either monotonically increasing or not earlier than one sample from the
 * newest sample requested from the object. Seeking backwards is efficient
 * in two cases: 1) seeking to position 0 (see: reset()) and 2) seeking to
 * the position previously declared as loop_start in init().
 */
int16_t AudioSampleData::operator[](uint32_t sampleNum)
{
    while (1) {
        ASSERT(mod);
        ASSERT(sampleNum < numSamples());

        // New sample
        if (sampleNum >= state.numSamples) {
            switch (mod->type) {

            case _SYS_PCM:
                decodeToSamplePCM(sampleNum);
                break;

            case _SYS_ADPCM:
                decodeToSampleADPCM(sampleNum);
                break;

            default:
                ASSERT(0);
            }
            return state.samples[sampleNum % kSampleBufSize];
        }

        // Cached sample
        if (sampleNum + kSampleBufSize >= state.numSamples) {
            return state.samples[sampleNum % kSampleBufSize];
        }

        // Oops, we need to rewind. Do we have an applicable snapshot?
        // (NB: Rollover in the subtraction is okay, it will cause this to be false)
        if (hasSnapshot() && sampleNum >= (snapshot.numSamples - kSampleBufSize)) {
            revertToSnapshot();
            return state.samples[sampleNum % kSampleBufSize];
        }

        // Last resort... Reset and retry.
        reset();
    }
}

// Save the next sample in the ringbuffer and update related instance data.
void AudioSampleData::writeNextSample(uint16_t sample)
{
    state.samples[(state.numSamples++) % kSampleBufSize] = sample;

    // Take a snapshot if necessary.
    if (!hasSnapshot() && state.numSamples == loopPoint)
        takeSnapshot();
}

void AudioSampleData::decodeToSampleSilence(uint32_t sampleNum)
{
    unsigned count = sampleNum + 1 - state.numSamples;
    ASSERT(count != 0);
    while (count--)
        writeNextSample(0);
}

void AudioSampleData::decodeToSamplePCM(uint32_t sampleNum)
{
    unsigned count = sampleNum + 1 - state.numSamples;
    ASSERT(count != 0);

    do {
        SvmMemory::PhysAddr pa;
        SvmMemory::VirtAddr va = mod->pData + state.bufPos;

        unsigned sampleCount = sampleNum + 1 - state.numSamples;
        uint32_t byteCount = sampleCount * sizeof(int16_t);

        if (!SvmMemory::mapROData(ref, va, byteCount, pa) ||
            !isAligned(pa, sizeof(int16_t))) {
            LOG((LGPFX "Memory mapping failure for PCM sample at VA 0x%08x\n", unsigned(va)));
            return decodeToSampleSilence(sampleNum);
        }

        const int16_t *samples = reinterpret_cast<const int16_t *>(pa);
        unsigned chunk = byteCount / sizeof(int16_t);
        chunk = MIN(chunk, count);
        ASSERT(chunk != 0);
        count -= chunk;
        state.bufPos += chunk * sizeof(int16_t);

        while (chunk--)
            writeNextSample(*(samples++));

    } while (count);
}

void AudioSampleData::decodeToSampleADPCM(uint32_t sampleNum)
{
    unsigned target = sampleNum + 1;
    ASSERT(target != state.numSamples);

    while (1) {
        SvmMemory::PhysAddr pa;
        SvmMemory::VirtAddr va = mod->pData + state.bufPos;

        unsigned sampleCount = sampleNum + 1 - state.numSamples;
        uint32_t byteCount = ceildiv<unsigned>(sampleCount, kNybblesPerByte);

        if (!SvmMemory::mapROData(ref, va, byteCount, pa)) {
            LOG((LGPFX "Memory mapping failure for ADPCM sample at VA 0x%08x\n", unsigned(va)));
            return decodeToSampleSilence(sampleNum);
        }

        uint8_t *ptr = pa;
        uint8_t *limit = pa + byteCount;

        do {
            writeNextSample(state.adpcm.decodeSample(&ptr));
            ASSERT(target >= state.numSamples);
            if (target == state.numSamples) {
                state.bufPos += ptr - pa;
                return;
            }
        } while (ptr < limit);
        ASSERT(ptr == limit);
        state.bufPos += byteCount;
    }
}
