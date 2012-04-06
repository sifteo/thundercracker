/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */
 
#include "audiosampledata.h"
#include "svmmemory.h"

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

void AudioSampleData::decodeToSample(uint32_t sampleNum)
{
    // decoders are not expected to decode backwards
    ASSERT(sampleNum >= oldestSample());

    FlashBlockRef ref;
    while(newestSample < sampleNum || newestSample == kNoSamples) {
        SvmMemory::PhysAddr pa;
        SvmMemory::VirtAddr va = mod->pData + bufPos;
        uint32_t bufLen = bytesForSamples(sampleNum - newestSample);
        if (!SvmMemory::mapROData(ref, va, bufLen, pa)) {
            LOG(("Could not map %p (length %d)!", (void *)va, sampleNum - newestSample));
            ASSERT(false);
            reset();
            return;
        }
        uint8_t *bufPtr = pa;

        while(bufPtr < pa + bufLen && (newestSample < sampleNum || newestSample == kNoSamples)) {
            switch(mod->type) {
                case _SYS_PCM:
                    ASSERT(pa + bufLen - bufPtr >= (uint8_t)sizeof(int16_t)); // XXX: seriously, sizeof() returns signed?
                    writeNextSample(*((int16_t *)bufPtr));
                    bufPtr += sizeof(int16_t);
                    break;
                case _SYS_ADPCM:
                    writeNextSample(adpcmDec.decodeSample(&bufPtr));
                    break;
            }
        }

        bufPos += bufPtr - pa;
    }
}