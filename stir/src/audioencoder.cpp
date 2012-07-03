/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "audioencoder.h"

#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>
#include <algorithm>
#include <vector>

using namespace std;


AudioEncoder *AudioEncoder::create(string name)
{
    transform(name.begin(), name.end(), name.begin(), ::tolower);

    if (name == "pcm")
        return new PCMEncoder();

    if (name == "adpcm" || name == "")
        return new ADPCMEncoder();

    return 0;
}

void PCMEncoder::encode(const std::vector<uint8_t> &in, std::vector<uint8_t> &out)
{
    // Already in PCM format
    out.insert(out.end(), in.begin(), in.end());
}

void ADPCMEncoder::encode(const std::vector<uint8_t> &in, std::vector<uint8_t> &out)
{
    State state;
    optimizeIC(state, in);
    encodeWithIC(state, in, out, in.size());
}

void ADPCMEncoder::optimizeIC(State &state, const std::vector<uint8_t> &in)
{
    /*
     * Find the best initial conditions for encoding a particular PCM sample.
     *
     * In the long run, the initial conditions don't matter as long as the
     * encoder and decoder agree. But sometimes it can take dozens of samples
     * for the CODEC to converge if the initial conditions are particularly
     * bad. Therefore we support storing a customized set of initial conditions
     * with each sample. This optimizer tries to find the best settings
     * to use for any particular sample.
     */

    // Too short to compress even?
    if (in.size() < 4) {
        state.sample = 0;
        state.index = 0;
        return;
    }

    // First sample
    state.sample = int16_t(in[0] | (in[1] << 8));

    // Try at most 100 samples
    unsigned inBytes = std::min<unsigned>(100, in.size());
    std::vector<uint8_t> dummy;

    /*
     * Pick the best initial index value.
     *
     * We can't leave this to the hill-climber below, since the index in ADPCM
     * is highly nonlinear. It's easy to find a rather terrible local minimum
     * in the error.
     *
     * So, for our initial guess, try every index value.
     */

    uint64_t error = -1;
    int bestIndex = 0;

    for (state.index = 0; state.index < INDEX_MAX; state.index++)  {
        uint64_t nextError = encodeWithIC(state, in, dummy, inBytes);
        if (nextError < error) {
            error = nextError;
            bestIndex = state.index;
        }
    }

    state.index = bestIndex;

    /*
     * Hill-climbing optimizer.
     *
     * At this point we're close to the best solution. Try making incremental
     * changes along each axis, and stop when there's no single change which
     * improves quality.
     */

    // Test a unit change in each direction on each axis
    for (;;) {
        uint64_t nextError;

        state.sample += 1;
        if ((nextError = encodeWithIC(state, in, dummy, inBytes)) < error) {
            error = nextError;
            continue;
        }
        state.sample -= 2;
        if ((nextError = encodeWithIC(state, in, dummy, inBytes)) < error) {
            error = nextError;
            continue;
        }
        state.sample += 1;

        if (state.index < INDEX_MAX) {
            state.index++;
            if ((nextError = encodeWithIC(state, in, dummy, inBytes)) < error) {
                error = nextError;
                continue;
            }
            state.index--;
        }

        if (state.index > 0) {
            state.index--;
            if ((nextError = encodeWithIC(state, in, dummy, inBytes)) < error) {
                error = nextError;
                continue;
            }
            state.index++;
        }

        break;
    }
}

uint64_t ADPCMEncoder::encodeWithIC(State state, const std::vector<uint8_t> &in,
    std::vector<uint8_t> &out, unsigned inBytes)
{
    /*
     * Using the provided initial conditions, encode some PCM data
     * to ADPCM, and calculate an error metric.
     *
     * The returned error is a 64-bit integer representation of
     * mean squared error, calculated by summing the squares of the
     * difference between actual sample and predictor state after
     * encoding each sample.
     *
     * If the input is not an even number of samples, the additional
     * bytes/samples will be truncated.
     */

    unsigned numPairs = inBytes / 4;
    const int16_t *inPtr = reinterpret_cast<const int16_t*>(&in[0]);
    uint64_t error = 0;

    out.resize(0);
    out.reserve(numPairs + HEADER_SIZE);

    // Write the initial conditions header
    out.push_back(state.sample);
    out.push_back(state.sample >> 8);
    out.push_back(state.index);

    while (numPairs--) {
        // Uncompressed samples
        int s1 = *(inPtr++);
        int s2 = *(inPtr++);
        
        // Compressed nybbles, and predictor errors
        unsigned n1 = encodeSample(state, s1);
        int e1 = state.sample - s1;
        unsigned n2 = encodeSample(state, s2);
        int e2 = state.sample - s2;

        // Update error metric
        error += e1*e1 + e2*e2;

        // Write out one byte (2 samples)
        out.push_back(n1 | (n2 << 4));
    }

    return error;
}

unsigned ADPCMEncoder::encodeSample(State &state, int sample)
{
    /*
     * Encode a single sample to a nybble of compressed data, and updates
     * the codec state in 'state'.
     *
     * Important: This isn't *quite* standard IMA ADPCM. The rounding
     * rules are a little different, in order to support a fast implementation
     * on ARM with multiply and shift.
     */
    
    static const uint16_t stepSizeTable[89] = {
        7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
        34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130,
        143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449,
        494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411,
        1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026,
        4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487,
        12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
    };

    static const int codeTable[16] = {
        0xFFFFFF01,
        0xFFFFFF03,
        0xFFFFFF05,
        0xFFFFFF07,
        0x00000209,
        0x0000040B,
        0x0000060D,
        0x0000080F,
        0xFFFFFFFF,
        0xFFFFFFFD,
        0xFFFFFFFB,
        0xFFFFFFF9,
        0x000002F7,
        0x000004F5,
        0x000006F3,
        0x000008F1,
    };

    int index = state.index;
    int step = stepSizeTable[index];

    // Difference between new sample and old prediction
    int prevSample = state.sample;
    int diff = sample - prevSample;

    // Find the best nybble for this diff
    unsigned bestCode = 0;
    int bestDiff = 0x100000;
    for (unsigned code = 0; code < 16; ++code) {
        int thisDiff = int(unsigned(int8_t(codeTable[code])) * unsigned(step)) >> 3;
        int thisError = std::max(thisDiff - diff, diff - thisDiff);
        int bestError = std::max(bestDiff - diff, diff - bestDiff);
        if (thisError <= bestError) {
            bestDiff = thisDiff;
            bestCode = code;
        }
    }

    // Update prediction
    state.sample = std::min(32767, std::max(-32768, prevSample + bestDiff));

    // Update quantizer step size
    index += (codeTable[bestCode] >> 8);
    state.index = std::min<int>(INDEX_MAX, std::max(0, index));

    return bestCode;
}
