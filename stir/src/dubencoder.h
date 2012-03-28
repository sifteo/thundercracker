/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _DUBENCODER_H
#define _DUBENCODER_H

#include <vector>

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
    DUBEncoder(unsigned width, unsigned height, unsigned frames);

    void encodeTile(uint16_t index);
    void logStats(Logger &log);
    void writeResult(std::vector<uint16_t> &data);
};


};  // namespace Stir

#endif
