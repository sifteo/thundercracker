/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
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

#ifndef _DUBENCODER_H
#define _DUBENCODER_H

#include "bits.h"
#include <vector>
#include <string>

namespace Stir {

class Logger;


/*
 * DUBEncoder --
 * 
 *    Encoder for a simple tile compression codec, named Dictionary
 *    Uniform Block. This codec divides assets into fixed size blocks
 *    of at most 8x8 tiles, which are all compressed independently.
 *    It is a dictionary-based codec, in which each tile's compression
 *    code is a back-reference to any other tile in the block.
 *
 *    To quickly locate the proper 8x8 block(s) in a large asset, the
 *    encoded data is prefixed with a block index, which lists the size
 *    of each compressed block.
 */

class DUBEncoder {
public:
    DUBEncoder(unsigned width, unsigned height, unsigned frames)
        : mWidth(width), mHeight(height), mFrames(frames) {}

    void encodeTiles(std::vector<uint16_t> &tiles);
    void logStats(const std::string &name, Logger &log);

    unsigned getTileCount() const;
    unsigned getCompressedWords() const;
    float getRatio() const;
    unsigned getNumBlocks() const;
    bool isTooLarge() const;
    bool isIndex16() const;
    void getResult(std::vector<uint16_t> &result) const;

private:
    static const unsigned BLOCK_SIZE;

    struct Code {
        enum { DELTA, REF, REPEAT, INVALID } type;
        int value;
    };

    unsigned mWidth, mHeight, mFrames;
    bool mIndex16;

    std::vector<uint16_t> blockResult;  // Compressed block data
    std::vector<uint16_t> indexResult;  // Word number, starting at blockResult[0]

    void encodeBlock(uint16_t *pTopLeft, unsigned width, unsigned height,
        std::vector<uint16_t> &blockData);
    Code findBestCode(const std::vector<uint16_t> &dict, uint16_t tile);

    unsigned getIndexSize() const;
    void debugCode(int x, int y, Code code, int tile) const;
    void packCode(Code code, BitBuffer &bits) const ;
    unsigned codeLen(Code code) const;
    unsigned packIndex(unsigned i) const;
};


};  // namespace Stir

#endif
