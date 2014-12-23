/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
 
#include "audiosampledata.h"
#include "svmmemory.h"
#include <algorithm>

#define LGPFX "AudioSampleData: "


void AudioSampleData::init(const _SYSAudioModule &mod)
{
    // Block boundary at which we take an automatic snapshot
    autoSnapshotPoint = mod.loopStart & ~HALF_BUFFER_MASK;
    snapshot.sampleNum = 0x7fffffff & ~HALF_BUFFER_MASK;

    // Load initial conditions for ADPCM
    if (mod.type == _SYS_ADPCM) {
        uint32_t buffer = 0;
        FlashBlockRef tempRef;
        SvmMemory::copyROData(tempRef, (SvmMemory::PhysAddr) &buffer, mod.pData, ADPCMState::HEADER_BYTES);
        adpcmIC.readHeader(buffer);
    }

    reset();
}

void AudioSampleData::fetchBlockPCM(uint32_t sampleNum, const _SYSAudioModule &mod)
{
    /*
     * PCM provides random access, and the data is already in the format we
     * need, so just do a copy from virtual memory to our buffer.
     */

    // Must be aligned to one half of the buffer
    ASSERT((sampleNum & HALF_BUFFER_MASK) == 0);

    int16_t *dest = &samples[sampleNum & FULL_BUFFER_MASK];
    ASSERT(dest + HALF_BUFFER <= &samples[FULL_BUFFER]);

    SvmMemory::VirtAddr va = mod.pData + (sampleNum * sizeof(int16_t));
    SvmMemory::PhysAddr pa = (SvmMemory::PhysAddr) dest;

    FlashBlockRef ref;
    SvmMemory::copyROData(ref, pa, va, HALF_BUFFER * sizeof(int16_t));

    // Update state (Ignore snapshots)
    state.sampleNum = sampleNum + HALF_BUFFER;
}

void AudioSampleData::fetchBlockADPCM(uint32_t sampleNum, const _SYSAudioModule &mod)
{
    /*
     * Starting from the current ADPCM codec state, decode one half-buffer
     * worth of aligned ADPCM samples.
     *
     * We also manage taking and restoring snapshots here.
     */

    // Argument is expected to be Half-buffer-aligned.
    ASSERT((sampleNum & HALF_BUFFER_MASK) == 0);

    // Fast local copy of ADPCM CODEC state (Either the last saved, or the initial conditions)
    unsigned stateSampleNum = state.sampleNum;
    ASSERT((stateSampleNum & HALF_BUFFER_MASK) == 0);
    ADPCMDecoder dec;
    dec.load(stateSampleNum ? state.adpcm : adpcmIC);

    FlashBlockRef ref;

    // Are we not decoding contiguously? May need to loop so we can skip forward.
    while (1) {

        if (UNLIKELY(stateSampleNum > sampleNum)) {
            // Need to skip backwards...

            if (sampleNum >= snapshot.sampleNum) {
                // Warp back to the snapshot
                dec.load(snapshot.adpcm);
                stateSampleNum = snapshot.sampleNum;

            } else {
                // Back to the beginning!
                dec.load(adpcmIC);
                stateSampleNum = 0;
            }
        }

        STATIC_ASSERT((HALF_BUFFER % NYBBLES_PER_BYTE) == 0);
        ASSERT((stateSampleNum & HALF_BUFFER_MASK) == 0);
        unsigned bytesRemaining = HALF_BUFFER / NYBBLES_PER_BYTE;
        SvmMemory::VirtAddr va = mod.pData + ADPCMState::HEADER_BYTES + (stateSampleNum / NYBBLES_PER_BYTE);

        int16_t *dest = &samples[stateSampleNum & FULL_BUFFER_MASK];
        ASSERT(dest + HALF_BUFFER <= &samples[FULL_BUFFER]);

        const uint32_t localAutoSnapshotPoint = autoSnapshotPoint;

        while (1) {
            uint32_t chunk = bytesRemaining;
            SvmMemory::PhysAddr pa;

            if (!SvmMemory::mapROData(ref, va, chunk, pa)) {
                LOG((LGPFX "Memory mapping failure for ADPCM sample at VA 0x%08x\n",
                    unsigned(va)));
                return;
            }

            STATIC_ASSERT(HALF_BUFFER <= 16);
            ASSERT(chunk <= 8);
            ASSERT(chunk > 0);

            switch (chunk) {
                case 8: dec.decodeByte(pa, dest);
                case 7: dec.decodeByte(pa, dest);
                case 6: dec.decodeByte(pa, dest);
                case 5: dec.decodeByte(pa, dest);
                case 4: dec.decodeByte(pa, dest);
                case 3: dec.decodeByte(pa, dest);
                case 2: dec.decodeByte(pa, dest);
                case 1: dec.decodeByte(pa, dest);
                case 0: break;
            };

            if (LIKELY(0 == (bytesRemaining -= chunk)))
                break;

            va += chunk;
        }
    
        // Next block...
        unsigned beginningOfBlock = stateSampleNum;
        stateSampleNum += HALF_BUFFER;

        // Save an automatic snapshot if applicable
        ASSERT((stateSampleNum & HALF_BUFFER_MASK) == 0);
        ASSERT((snapshot.sampleNum & HALF_BUFFER_MASK) == 0);
        if (UNLIKELY(stateSampleNum == localAutoSnapshotPoint)) {
            snapshot.sampleNum = stateSampleNum;
            dec.store(snapshot.adpcm);
        }

        // We just decoded the block we were looking for?
        if (LIKELY(sampleNum == beginningOfBlock))
            break;
    }

    // Save new codec state
    state.sampleNum = stateSampleNum;
    dec.store(state.adpcm);
}
