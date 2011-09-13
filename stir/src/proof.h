/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
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
    std::ofstream mStream;

    void head();

    void defineTiles(const TilePool &pool);
    void tileRange(unsigned begin, unsigned end);
    void tileGrid(const TileGrid &grid);
};


};  // namespace Stir

#endif
