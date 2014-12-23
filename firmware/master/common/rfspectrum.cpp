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

#include "rfspectrum.h"
#include "radio.h"
#include "prng.h"

#include "string.h"

const unsigned RFSpectrumModel::MAX_RETRIES =
        PacketTransmission::DEFAULT_HARDWARE_RETRIES *
        PacketTransmission::DEFAULT_SOFTWARE_RETRIES;

const uint32_t RFSpectrumModel::K =
        RFSpectrumModel::ONE * (MAX_RETRIES / WIFI_CH_WIDTH);

void RFSpectrumModel::init()
{
    memset(buckets, 0, sizeof buckets);
}

void RFSpectrumModel::update(unsigned channel, unsigned retryCount)
{
    unsigned bucket = channel >> 1;

    ASSERT(retryCount <= MAX_RETRIES);
    ASSERT(bucket < NUM_BUCKETS);

    for (unsigned i = 0; i < NUM_BUCKETS; ++i) {

        uint32_t sf = scaleFactor(bucket, i);
        uint16_t &bucketVal = buckets[i];

        // fixed point 16.16 linear interpolation
        int64_t result64 = static_cast<int64_t>((bucketVal * (ONE - sf)) + (retryCount * sf));
        bucketVal = result64 >> FIXPT_SHIFT;
    }
}

unsigned RFSpectrumModel::suggestChannel()
{
    /*
     * Find the channel least likely to be noisy at the moment.
     *
     * Should only be called in ISR context, as we access RadioManager::prngISR.
     *
     * Start from a randomized channel, because in the event that we don't have
     * a very well populated model (ie, on startup, or if we only have a couple
     * cubes connected), we might have several buckets with 0 energy, but we'd
     * like to spread the cubes out as much as possible to improve our model.
     */

    PRNG::collectTimingEntropy(&RadioManager::prngISR);

    unsigned ch = PRNG::valueBounded(&RadioManager::prngISR, NUM_BUCKETS - 1);
    unsigned energy = buckets[ch];

    unsigned chIter = ch;
    for (unsigned i = 0; i < NUM_BUCKETS; ++i) {
        if (buckets[chIter] < energy) {
            energy = buckets[chIter];
            ch = chIter;
        }
        chIter = (chIter + 1) % NUM_BUCKETS;
    }

    /*
     * buckets are 2 channels wide, so it's a bit arbitrary which one of them
     * we actually suggest, once we've chosen the bucket.
     */
    ch *= 2;
    ch += (PRNG::value(&RadioManager::prngISR) & 0x1);

    ASSERT(ch <= MAX_RF_CHANNEL);

    return ch;
}
