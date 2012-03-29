/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <assert.h>
#include "dubencoder.h"
#include "logger.h"
using namespace Stir;

const unsigned DUBEncoder::BLOCK_SIZE = 8;


void DUBEncoder::encodeTiles(std::vector<uint16_t> &tiles)
{
    BitBuffer bits;

    unsigned numBlocks = ((mWidth + BLOCK_SIZE - 1) / BLOCK_SIZE) *
                         ((mHeight + BLOCK_SIZE - 1) / BLOCK_SIZE) *
                         mFrames;

    // Reserve space for our index
    result.resize(numBlocks);

    unsigned blockIndex = 0;
    for (unsigned f = 0; f < mFrames; f++)
        for (unsigned y = 0; y < mHeight; y += BLOCK_SIZE)
            for (unsigned x = 0; x < mWidth; x += BLOCK_SIZE) {
                unsigned w = std::min(BLOCK_SIZE, mWidth - x);
                unsigned h = std::min(BLOCK_SIZE, mHeight - y);

                result[blockIndex++] = result.size();
                encodeBlock(&tiles[x + y*mWidth + f*mWidth*mHeight], w, h, result);
            }
}

unsigned DUBEncoder::getNumBlocks() const
{
    return ((mWidth + BLOCK_SIZE - 1) / BLOCK_SIZE) *
           ((mHeight + BLOCK_SIZE - 1) / BLOCK_SIZE) *
           mFrames;
}

bool DUBEncoder::isTooLarge() const
{
    return result.size() >= 0x10000;
}

unsigned DUBEncoder::getTileCount() const
{
    return mWidth * mHeight * mFrames;
}

unsigned DUBEncoder::getCompressedWords() const
{
    return result.size();
}

float DUBEncoder::getRatio() const
{
    return 100.0 - getCompressedWords() * 100.0 / getTileCount();
}

void DUBEncoder::logStats(const std::string &name, Logger &log)
{
    log.infoLineWithLabel(name.c_str(),
        "%4d tiles, %4d words, % 5.01f%% compression",
        getTileCount(), getCompressedWords(), getRatio());
}

void DUBEncoder::encodeBlock(uint16_t *pTopLeft,
    unsigned width, unsigned height, std::vector<uint16_t> &data)
{
    BitBuffer bits;
    std::vector<uint16_t> dict;
    Code prevCode = { Code::INVALID };
    unsigned repeatCount = 0;
    bool repeating = false;

    for (unsigned y = 0; y < height; y++)
        for (unsigned x = 0; x < width; x++) {
            uint16_t tile = pTopLeft[x + y*mWidth];

            // Find the best code for this tile, and update the dictionary
            Code code = findBestCode(dict, tile);
            dict.push_back(tile);

            // If we ever output two identical codes in a row, that counts
            // as a run. The next code *must* be a REPEAT code.
            bool sameCode = (code.type == prevCode.type && code.value == prevCode.value);
            prevCode = code;

            if (repeating) {
                if (sameCode) {
                    // Extending an existing run
                    repeatCount++;
                    continue;
                } else {
                    // Break an existing run
                    Code rep = { Code::REPEAT, repeatCount };
                    debugCode(rep);
                    packCode(rep, bits);
                    bits.flush(data);
                    repeating = false;
                }
            } else if (sameCode) {
                // Beginning a run. The next code will be a REPEAT.
                repeating = true;
                repeatCount = 0;
            }

            debugCode(code);
            packCode(code, bits);
            bits.flush(data);
        }

    if (repeating) {
        // Flush any final REPEAT code we have stowed away.
        Code rep = { Code::REPEAT, repeatCount };
        debugCode(rep);
        packCode(rep, bits);
    }

    // Flush all remaining data, padding to a word boundary.
    bits.flush(data, true);
}

void DUBEncoder::debugCode(DUBEncoder::Code c)
{
#if 0
    printf("{%d,%d}\n", c.type, c.value);
#endif
}

DUBEncoder::Code DUBEncoder::findBestCode(const std::vector<uint16_t> &dict, uint16_t tile)
{
    Code code;
    unsigned bestLength;

    if (dict.empty()) {
        // Dictionary is empty. This is a special case in which DELTA
        // codes are literal. In effect, the nonexistant last entry in
        // the dict is treated as zero.

        code.type = Code::DELTA;
        code.value = tile;
        bestLength = codeLen(code);

    } else {
        // Try a DELTA code based on the most recent dictionary entry.

        int delta = (int)tile - (int)dict.back();
        code.type = Code::DELTA;
        code.value = delta;
        bestLength = codeLen(code);
    }

    // Now see if we can do better by scanning for an identical tile
    // in our history, and emitting a REF code.

    for (unsigned i = 0; i < dict.size(); i++)
        if (tile == dict[dict.size() - 1 - i]) {
            Code candidate = { Code::REF, i };
            unsigned len = codeLen(candidate);
            if (len < bestLength) {
                code = candidate;
                bestLength = len;
                
                // We won't do better by increasing 'i' at this point.
                break;
            }
        }

    return code;
}

void DUBEncoder::packCode(Code code, BitBuffer &bits)
{
    // Experimentally determined sweet-spot
    const unsigned chunk = 3;

    switch (code.type) {

    case Code::DELTA:
        // Type bit, sign bit, delta
        bits.append(0, 1);
        if (code.value < 0) {
            bits.append(1, 1);
            bits.appendVar(-code.value, chunk);
        } else {
            bits.append(0, 1);
            bits.appendVar(code.value, chunk);
        }
        break;

    case Code::REF:
        // Type bit, backref index
        bits.append(1, 1);
        bits.appendVar(code.value, chunk);
        break;

    case Code::REPEAT:
        // Repeat count only, no header.
        // This code only appears after two repeated codes.
        bits.appendVar(code.value, chunk);
        break;

    case Code::INVALID:
        assert(0);
        break;
    }
}

unsigned DUBEncoder::codeLen(Code code)
{
    BitBuffer bits;
    packCode(code, bits);
    return bits.getCount();
}
