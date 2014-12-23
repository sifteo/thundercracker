/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc.
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

#ifndef _PROOF_H
#define _PROOF_H

#include <stdint.h>
#include <string>
#include <vector>
#include <fstream>

#include "tile.h"
#include "script.h"
#include "logger.h"

namespace Stir {

/*
 * Encoding utilities
 */

void Base64Encode(const std::vector<uint8_t> &data, std::string &out);
void DataURIEncode(const std::vector<uint8_t> &data, const std::string &mime, std::string &out);
void TileURIEncode(const Tile &t, std::string &out);
std::string HTMLEscape(const std::string &s);


/*
 * ProofWriter --
 *
 *     Writes out a self-contained HTML file that visualizes the
 *     results of a particular STIR processing run.
 */

class ProofWriter {
 public:
    ProofWriter(Logger &log, const char *filename);

    void writeGroup(const Group &group);
    void close();

 private:
    static const char *header;

    Logger &mLog;
    unsigned mID;
    std::ofstream mStream;

    void defineTiles(const TilePool &pool);
    unsigned newCanvas(unsigned tilesW, unsigned tilesH, unsigned tileSize, bool hidden=false);
    void tileRange(unsigned begin, unsigned end, unsigned tileSize, unsigned width);
    void tileGrid(const TileGrid &grid, unsigned tileSize, bool hidden=false);
};


};  // namespace Stir

#endif
