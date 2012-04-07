/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */
 
#include "audiosampledata.h"
#include "svmmemory.h"

/* Providing an array interface to sample data is convenient for the audio
 * channel, but it's a bit misleading because truly random access patterns
 * can incur massive performance penalties. In general, requests should be
 * either monotonically increasing or not earlier than one sample from the
 * newest sample requested from the object. Seeking backwards is efficient
 * in two cases: 1) seeking to position 0 (see: reset()) and 2) seeking to
 * the position previously declared as loop_start in init().
 */
int16_t AudioSampleData::operator[](uint32_t sampleNum) {
    ASSERT(mod);
    ASSERT(sampleNum < numSamples());

    if (sampleNum < oldestSample()) {
        if (hasSnapshot() && sampleNum > snapshotData.newestSample - kSampleBufSize) {
            revert();
        } else {
            reset();
        }
    }

    if (sampleNum > newestSample || newestSample == kNoSamples)
        decodeToSample(sampleNum);

    ASSERT(sampleNum <= newestSample && sampleNum >= oldestSample());

    return samples[((sampleNum - oldestSample()) + ringPos) % arraysize(samples)];
}

// Compute the expected number of bytes needed to store a number of samples.
uint32_t AudioSampleData::bytesForSamples(uint32_t samples) const {
    switch(mod->type) {
        case _SYS_PCM:
            return samples * sizeof(int16_t);

        case _SYS_ADPCM:
            // round up!
            return (samples + (kNibblesPerByte - 1)) / kNibblesPerByte;
    }
    ASSERT(mod->type == _SYS_PCM || mod->type == _SYS_ADPCM);
    return 0;
}

// Save the next sample in the ringbuffer and update related instance data.
void AudioSampleData::writeNextSample(uint16_t sample) {
    samples[ringPos++] = sample;
    ringPos %= arraysize(samples);
    if(newestSample == kNoSamples) newestSample = 0;
    else newestSample++;
    // Take a snapshot if necessary.
    if (!hasSnapshot() && oldestSample() == loopStart && loopStart > 0) {
        snapshot();
    }
}

void AudioSampleData::decodeToSample(uint32_t sampleNum)
{
    // Decoders are not expected to decode backwards.
    ASSERT(sampleNum >= oldestSample());

    while(newestSample < sampleNum || newestSample == kNoSamples) {
        SvmMemory::PhysAddr pa;
        SvmMemory::VirtAddr va = mod->pData + bufPos;
        uint32_t bufLen = bytesForSamples(sampleNum - newestSample);
        if (!SvmMemory::mapROData(ref, va, bufLen, pa)) {
            // Fail in as many ways as possible!
            LOG((LGPFX"Could not map %p (length %d)!\n",
                 (void *)va, sampleNum - newestSample));
            ASSERT(false);
            // The best we can now is be quiet and get out of the way.
            while(newestSample < sampleNum || newestSample == kNoSamples)
                writeNextSample(0);
            return;
        }

        uint8_t *bufPtr = pa;
        switch(mod->type) {
            case _SYS_PCM:
                while(bufPtr < pa + bufLen && (newestSample < sampleNum || newestSample == kNoSamples)) {
                    ASSERT(pa + bufLen - bufPtr >= (uint8_t)sizeof(int16_t));
                    writeNextSample(*((int16_t *)bufPtr));
                    bufPtr += sizeof(int16_t);
                }
                break;
            case _SYS_ADPCM:
                while(bufPtr < pa + bufLen && (newestSample < sampleNum || newestSample == kNoSamples)) {
                    writeNextSample(adpcmDec.decodeSample(&bufPtr));
                }
                break;
        }
        bufPos += bufPtr - pa;
    }
}

/*
 *                            _           _       
 *  ___ _ __   __ _ _ __  ___| |__   ___ | |_ ___ 
 * / __| '_ \ / _` | '_ \/ __| '_ \ / _ \| __/ __|
 * \__ \ | | | (_| | |_) \__ \ | | | (_) | |_\__ \
 * |___/_| |_|\__,_| .__/|___/_| |_|\___/ \__|___/
 *                 |_|                            
 */
void AudioSampleData::snapshot() {
    memcpy(snapshotData.samples, samples, sizeof(samples));
    snapshotData.newestSample = newestSample;
    snapshotData.ringPos = ringPos;
    snapshotData.bufPos = bufPos;
    snapshotData.adpcmDec = adpcmDec;
}

bool AudioSampleData::hasSnapshot() {
    return snapshotData.newestSample > 0;
}

void AudioSampleData::revert() {
    memcpy(samples, snapshotData.samples, sizeof(samples));
    newestSample = snapshotData.newestSample;
    ringPos = snapshotData.ringPos;
    bufPos = snapshotData.bufPos;
    adpcmDec = snapshotData.adpcmDec;
}
