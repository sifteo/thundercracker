/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "proof.h"
#include "lodepng.h"

namespace Stir {

void Base64Encode(const std::vector<uint8_t> &data, std::string &out)
{
    /*
     * A very simple MIME Base64 encoder.
     */

    static const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    for (unsigned i = 0; i < data.size(); i += 3) {
        unsigned len = data.size() - i;
        uint8_t b1 = data[i];
        uint8_t b2 = len >= 2 ? data[i+1] : 0;
        uint8_t b3 = len >= 3 ? data[i+2] : 0;
        uint32_t word = (b1 << 16) | (b2 << 8) | b3;

        out.append(1, charset[(word >> (6*3)) & 63]);
        out.append(1, charset[(word >> (6*2)) & 63]);
        out.append(1, len >= 2 ? charset[(word >> (6*1)) & 63] : '=');
        out.append(1, len >= 3 ? charset[(word >> (6*0)) & 63] : '=');
    }
}

void DataURIEncode(const std::vector<uint8_t> &data, const std::string &mime, std::string &out)
{
    /*
     * Encode a data: URI. with the specified data and MIME type.
     */

    out.append("data:");
    out.append(mime);
    out.append(";base64,");
    Base64Encode(data, out);
}

void TileURIEncode(const Tile &t, std::string &out)
{
    /*
     * Encode a tile image as a data URI.
     *
     * We scale it up, in order to effectively counteract the
     * browser's image filtering and give us nearest-neighbor even
     * when zoomed in.
     */

    const unsigned scale = 4;
    const unsigned pixels = scale * Tile::SIZE;

    LodePNG::Encoder encoder;
    std::vector<uint8_t> png;
    std::vector<uint8_t> image;

    image.reserve(4 * pixels * pixels);

    for (unsigned y = 0; y < pixels; y++)
        for (unsigned x = 0; x < pixels; x++) {
            RGB565 color = t.pixel(x / scale, y / scale);
            image.push_back(color.red());
            image.push_back(color.green());
            image.push_back(color.blue());
            image.push_back(0xFF);
        }

    encoder.encode(png, &image[0], pixels, pixels);
    DataURIEncode(png, "image/png", out);
}

std::string HTMLEscape(const std::string &s)
{
    std::string out;

    for (unsigned i = 0; i < s.length(); i++) {
        char c = s[i];
 
        if (c == '&')
            out += "&amp;";
        else if (c == '>')
            out += "&gt;";
        else if (c == '<')
            out += "&lt;";
        else
            out.append(1, c);
    }
    
    return out;
}


ProofWriter::ProofWriter(Logger &log, const char *filename)
    : mLog(log), mID(0)
{
    if (filename) {
        mStream.open(filename);
        if (!mStream.is_open())
            log.error("Error opening proof file '%s'", filename);
    }
    
    if (mStream.is_open()) {
        mStream.setf(std::ios::fixed, std::ios::floatfield);  
        mStream.precision(2);   

        mStream << header;
    }
}
 
void ProofWriter::writeGroup(const Group &group)
{
    if (!mStream.is_open())
        return;

    mLog.taskBegin("Generating proof");
    
    mStream <<
        "<h1>" << HTMLEscape(group.getName()) << "</h1>\n"
        "<p>\n"
               << group.getPool().size() << " tiles, "
               << group.getLoadstream().size() / 1024.0 << " kB stream\n"
        "</p>\n";

    defineTiles(group.getPool());

    tileRange(0, group.getPool().size(), Tile::SIZE, 96);

    for (std::set<Image*>::iterator i = group.getImages().begin();
         i != group.getImages().end(); i++) {

        Image *image = *i;

        mStream << "<h2>" << HTMLEscape(image->getName());

        unsigned firstId = mID + 1;
        unsigned idCount = image->getGrids().size();
        if (idCount > 1) {
            mStream << "<span class=\"button\" id=\"buttonAll" << firstId
                    << "\" onclick=\"toggleDisplayMode(" << firstId << ", " << idCount << ", \'all\')\">all</span>";

            mStream << "<span class=\"button\" id=\"buttonAnim" << firstId
                    << "\" onclick=\"toggleDisplayMode(" << firstId << ", " << idCount << ", \'anim\')\">play</span>";
        }

        mStream << "</h2>\n";

        bool hidden = false;
        for (std::vector<TileGrid>::const_iterator j = image->getGrids().begin();
             j != image->getGrids().end(); j++) {

            tileGrid(*j, Tile::SIZE * 2, hidden);
            hidden = true;
        }
    }

    mLog.taskEnd();
}

void ProofWriter::close()
{
    if (mStream.is_open()) {
        mStream << "</body></html>\n";
        mStream.close();
    }
}

void ProofWriter::defineTiles(const TilePool &pool)
{
    /*
     * Define a set of tile data, in Javascript. Tiles are represented
     * as a common prefix (to optimize out the PNG header) plus an
     * array of tile-specific suffixes.
     */

    if (!pool.size())
        return;

    std::vector<std::string> uri(pool.size());

    for (unsigned i = 0; i < pool.size(); i++)
        TileURIEncode(*pool.tile(i), uri[i]);

    // Find the longest common prefix
    unsigned prefixLen = uri[0].length();
    for (unsigned i = 0; i < pool.size(); i++)
        while (prefixLen && uri[i].compare(0, prefixLen, uri[0], 0, prefixLen))
            prefixLen--;

    mStream << "<script>pool = defineTiles(\"" << uri[0].substr(0, prefixLen) << "\",[";

    for (unsigned i = 0; i < pool.size(); i++)
        mStream << "\"" << uri[i].substr(prefixLen) << "\",";

    mStream << "]);</script>\n";
}

unsigned ProofWriter::newCanvas(unsigned tilesW, unsigned tilesH, unsigned tileSize, bool hidden)
{
    mID++;

    mStream << "<canvas id=\"i" << mID << "\" width=\""
            << (tilesW * tileSize) << "px\" height=\""
            << (tilesH * tileSize) << "px\" ";

    if (hidden)
        mStream << "style=\"display: none;\" ";

    mStream << "></canvas>";

    return mID;
}

void ProofWriter::tileRange(unsigned begin, unsigned end, unsigned tileSize, unsigned width)
{
    unsigned height = ((end - begin) + (width - 1)) / width;
    unsigned id = newCanvas(width, height, tileSize);

    mStream << "<script>(new TileGrid(pool, " << id << ", " << tileSize
            << ")).range(" << begin << ", " << end << ");</script>";
}

void ProofWriter::tileGrid(const TileGrid &grid, unsigned tileSize, bool hidden)
{
    unsigned id = newCanvas(grid.width(), grid.height(), tileSize, hidden);

    mStream << "<script>(new TileGrid(pool, " << id << ", " << tileSize
            << ")).array([";

    for (unsigned y = 0; y < grid.height(); y++)
        for (unsigned x = 0; x < grid.width(); x++) {
            TilePool::Index index = grid.getPool().index(grid.tile(x, y));
            mStream << index << ",";
        }

    mStream << "]);</script>";
}


};  // namespace Stir
