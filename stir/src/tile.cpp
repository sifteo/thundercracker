/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * Micah Elizabeth Scott <micah@misc.name>
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

#include <assert.h>
#include <stdio.h>
#include <float.h>
#include <algorithm>
#include <set>
#include <map>
#include <math.h>

#include "tile.h"
#include "tilecodec.h"


/*
 * Tile hash -- A 32-bit FNV-1 hash computed over all pixels.
 * See: http://isthe.com/chongo/tech/comp/fnv/
 */
namespace std {
    namespace tr1 {
        template<> struct hash<Stir::Tile::Identity> {
            std::size_t operator()(Stir::Tile::Identity const &key) const {         
                uint32_t h = 2166136261UL;
                unsigned len = sizeof(key.pixels) / sizeof(uint32_t);
                uint32_t *data = (uint32_t*) &key.pixels[0];
                
                do {
                    h ^= *(data++);
                    h *= 16777619UL;
                } while (--len);
                
                return h;
            }
        };
    }
}

namespace Stir {

std::tr1::unordered_map<Tile::Identity, TileRef> Tile::instances;

Tile::Tile(const Identity &id)
    : mHasSobel(false), mHasDec4(false), mID(id)
    {}

TileRef Tile::instance(const Identity &id)
{
    /*
     * Return an existing Tile matching the given identity, or create a new one if necessary.
     */
     
    std::tr1::unordered_map<Identity, TileRef>::iterator i = instances.find(id);
        
    if (i == instances.end()) {
        TileRef tr(new Tile(id));
        instances[id] = tr;
        return tr;
    } else {
        return i->second;
    }
}

TileRef Tile::instance(const TileOptions &opt, uint8_t *rgba, size_t stride)
{
    /*
     * Load a tile image from a full-color RGBA source bitmap.
     */

    Identity id;
    const uint8_t alphaThreshold = 0x80;
    uint8_t *row, *pixel;
    unsigned x, y;
    
    id.options = opt;

    RGB565 *dest = id.pixels;
    for (row = rgba, y = SIZE; y; --y, row += stride)
        for (pixel = row, x = SIZE; x; --x, pixel += 4) {
            RGB565 color = RGB565(pixel);

            if (!id.options.chromaKey) {
                // No transparency in the image, we're allowed to use any color.
                *dest = color;
            }
            else if (pixel[3] < alphaThreshold) {
                /*
                 * Pixel is actually transparent.
                 *
                 * Use CHROMA_KEY as the MSB, and encode some information
                 * in the LSB. Currently, the firmware wants to know if the
                 * rest of this scanline on the current tile is entirely
                 * transparent. If so, we set the End Of Line bit.
                 */

                dest->value = CHROMA_KEY << 8;
                
                bool eol = true;
                for (unsigned xr = 0; xr < x; xr++) {
                    if (pixel[3 + xr*4] >= alphaThreshold) {
                        eol = false;
                        break;
                    }
                }
                
                if (eol)
                    dest->value |= CKEY_BIT_EOL;
            }
            else if ((color.value >> 8) == CHROMA_KEY) {
                /*
                 * Pixel isn't transparent, but it would look like
                 * the chromakey to our firmware's 8-bit comparison.
                 * Modify the color slightly (toggle the red LSB).
                 */

                *dest = (uint16_t)(color.value ^ (1 << 11));
            }
            else {
                // Opaque pixel
                *dest = color;
            }

            dest++;
        }    

    return instance(id);
}

double TileOptions::getMaxMSE() const
{
    /*
     * Convert from the user-visible "quality" to a maximum MSE limit.
     * User-visible quality numbers are in the range [0, 10]. Larger
     * is better. I've tried to scale this so that it's intuitively
     * similar to the quality metrics used by e.g. JPEG.
     */

    double err = 10.0 - std::min(10.0, std::max(0.0, quality));
    return err * 500;
}

void Tile::constructPalette(void)
{
    /*
     * Create a TilePalette for this particular tile. If the tile has few enough colors
     * that we can make a LUT, the palette will include a populated LUT. Additionally,
     * as a heuristic to help us provide color matches across transitions from higher
     * tile color depth to lower color depth, we try to order the most popular colors
     * at the begining of the LUT.
     */

    // Tracks unique colors, and their frequencies
    std::map<RGB565, unsigned> colors;

    for (unsigned i = 0; i < PIXELS; i++)
        colors[mID.pixels[i]] = colors[mID.pixels[i]] + 1;

    mPalette.numColors = colors.size();

    if (mPalette.hasLUT()) {
        // Sort the colors using an inverse mapping.

        std::multimap<unsigned, RGB565> lutSorter;
        for (std::map<RGB565, unsigned>::iterator i = colors.begin(); i != colors.end(); i++)
            lutSorter.insert(std::pair<unsigned, RGB565>(i->second, i->first));

        unsigned index = 0;
        for (std::multimap<unsigned, RGB565>::reverse_iterator i = lutSorter.rbegin();
             i != lutSorter.rend(); i++)
            mPalette.colors[index++] = i->second;
    }
}

void Tile::constructSobel()
{
    /*
     * Build a Sobel gradient approximation matrix for this tile.
     *
     * See: http://en.wikipedia.org/wiki/Sobel_operator
     */

    mHasSobel = true;
    mSobelTotal = 0;

    unsigned i = 0;
    for (unsigned y = 0; y < SIZE; y++)
        for (unsigned x = 0; x < SIZE; x++, i++) {

            // Luminance of eight neighbor pixels
            float l00 = CIELab(pixelWrap(x-1, y-1)).L;
            float l10 = CIELab(pixelWrap(x  , y-1)).L;
            float l20 = CIELab(pixelWrap(x+1, y-1)).L;
            float l01 = CIELab(pixelWrap(x-1, y  )).L;
            float l21 = CIELab(pixelWrap(x+1, y  )).L;
            float l02 = CIELab(pixelWrap(x-1, y+1)).L;
            float l12 = CIELab(pixelWrap(x  , y+1)).L;
            float l22 = CIELab(pixelWrap(x+1, y+1)).L;

            mSobelGx[i] = -l00 +l20 -l01 -l01 +l21 +l21 -l02 +l22;
            mSobelGy[i] = -l00 +l02 -l10 -l10 +l12 +l12 -l20 +l22;

            mSobelTotal += mSobelGx[i] * mSobelGx[i];
            mSobelTotal += mSobelGy[i] * mSobelGy[i];
        }

#ifdef DEBUG_SOBEL
    for (i = 0; i < PIXELS; i++) {
        int x = std::max(0, std::min(255, (int)(128 + mSobelGx[i])));
        int y = std::max(0, std::min(255, (int)(128 + mSobelGy[i])));
        mPixels[i] = RGB565(x, y, (x+y)/2);
    }
#endif
}    

void Tile::constructDec4()
{
    /*
     * Build a decimated 2x2 pixel representation of this tile, using
     * averaged CIELab colors.
     */

    const unsigned scale = SIZE / 2;
    unsigned i = 0;

    mHasDec4 = true;

    for (unsigned y1 = 0; y1 < SIZE; y1 += scale)
        for (unsigned x1 = 0; x1 < SIZE; x1 += scale) {
            CIELab acc;

            // Y/X pixels
            for (unsigned y2 = y1; y2 < y1 + scale; y2++)
                for (unsigned x2 = x1; x2 < x1 + scale; x2++)
                    acc += pixel(x2, y2);
            
            acc /= scale * scale;
            mDec4[i++] = acc;
        }

#ifdef DEBUG_DEC4
    for (unsigned y = 0; y < SIZE; y++)
        for (unsigned x = 0; x < SIZE; x++)
            mPixels[x + (y * SIZE)] = mDec4[x/scale + y/scale * 2].rgb();
#endif
}

double Tile::errorMetric(Tile &other, double limit)
{
    /*
     * This is a rather ad-hoc attempt at a perceptually-optimized
     * error metric. It is a form of multi-scale MSE metric, plus we
     * attempt to take into account the geometric structure of a tile
     * by comparing a luminance edge detection map.
     *
     * If we exceed 'limit', the test can exit early. This lets us
     * calculate the easy metrics first, and skip the rest if we're
     * already over.
     */

    double error = 0;

    error += 0.450 * coarseMSE(other);
    if (error > limit)
        return DBL_MAX;

    error += 0.025 * fineMSE(other);
    if (error > limit)
        return DBL_MAX;

    error += 5.00 * sobelError(other);

    return error * 60.0;
}

double Tile::fineMSE(Tile &other)
{
    /*
     * A normal pixel-wise mean squared error metric.
     */

    double error = 0;

    for (unsigned i = 0; i < PIXELS; i++)
        error += CIELab(mID.pixels[i]).meanSquaredError(CIELab(other.mID.pixels[i]));

    return error / PIXELS;
}

double Tile::coarseMSE(Tile &other)
{
    /*
     * A reduced scale MSE metric using the 2x2 pixel decimated version of our tile.
     */

    double error = 0;

    if (!mHasDec4)
        constructDec4();
    if (!other.mHasDec4)
        other.constructDec4();

    for (unsigned i = 0; i < 4; i++)
        error += mDec4[i].meanSquaredError(other.mDec4[i]);

    return error / 4;
}

double Tile::sobelError(Tile &other)
{
    /*
     * An error metric based solely on detecting structural luminance
     * differences using the Sobel operator.
     */

    double error = 0;

    if (!mHasSobel)
        constructSobel();
    if (!other.mHasSobel)
        other.constructSobel();

    for (unsigned i = 0; i < PIXELS; i++) {
        double gx = mSobelGx[i] - other.mSobelGx[i];
        double gy = mSobelGy[i] - other.mSobelGy[i];

        error += gx * gx + gy * gy;
    }
    
    // Contrast difference over total contrast
    return error / (1 + mSobelTotal + other.mSobelTotal);
}

TileRef Tile::reduce(ColorReducer &reducer) const
{
    /*
     * Reduce a tile's color palette, using a completed optimized
     * ColorReducer instance.
     */

    Identity reduced;   
    reduced.options = mID.options;

    for (unsigned i = 0; i < PIXELS; i++) {
        RGB565 original = mID.pixels[i];
        
        if ((original.value >> 8) == CHROMA_KEY) {
            // Don't touch it if this is a special keyed color
            reduced.pixels[i] = original;
        } else {
            reduced.pixels[i] = reducer.nearest(original);
        }
    }

    return instance(reduced);
}

TilePalette::TilePalette()
    : numColors(0)
    {}

const char *TilePalette::colorModeName(ColorMode m)
{
    switch (m) {
    case CM_LUT1:       return "CM_LUT1";
    case CM_LUT2:       return "CM_LUT2";
    case CM_LUT4:       return "CM_LUT4";
    case CM_LUT16:      return "CM_LUT16";
    case CM_TRUE:       return "CM_TRUE";
    default:            return "<invalid>";
    }
}

TileStack::TileStack()
    : index(NO_INDEX), mPinned(false), mLossless(false)
    {}

void TileStack::add(TileRef t)
{
    const double epsilon = 1e-3;
    double maxMSE = t->options().getMaxMSE();

    if (maxMSE < epsilon) {
        // Lossless. Replace the entire stack, yielding a trivial median.
        tiles.clear();
        mLossless = true;
    }

    // Add to stack, invalidating cache. (Don't modify lossless stacks)
    if (tiles.empty() || !mLossless) {
        tiles.push_back(t);
        cache = TileRef();
    }

    // A stack with any pinned tiles in it is itself pinned.
    if (t->options().pinned)
        mPinned = true;
}

void TileStack::replace(TileRef t)
{
    // Replace the entire stack with a single new tile.

    tiles.clear();
    add(t);
    cache = t;
}

void TileStack::computeMedian()
{
    Tile::Identity median;
    std::vector<RGB565> colors(tiles.size());

    // The median algorithm repeats independently for every pixel in the tile.
    for (unsigned i = 0; i < Tile::PIXELS; i++) {

        // Collect possible colors for this pixel
        for (unsigned j = 0; j < tiles.size(); j++)
            colors[j] = tiles[j]->pixel(i);

        // Sort along the major axis
        int major = CIELab::findMajorAxis(&colors[0], colors.size());
        std::sort(colors.begin(), colors.end(),
                  CIELab::sortAxis(major));

        // Pick the median color
        median.pixels[i] = colors[colors.size() >> 1];
    }

    cache = Tile::instance(median);

    /*
     * Some individual tiles will receive a huge number of references.
     * This is a bit of a hack to prevent a single tile stack from growing
     * boundlessly. If we just calculated a median for a stack over a preset
     * maximum size, we replace the entire stack with copies of the median
     * image, in order to give that median significant (but not absolute)
     * statistical weight.
     *
     * The problem with this is that a tile's median is no longer
     * computed globally across the entire asset group, but is instead
     * more of a sliding window. But for the kinds of heavily repeated
     * tiles that this algorithm will apply to, maybe this isn't an
     * issue?
     */

    if (tiles.size() > MAX_SIZE) {
        tiles = std::vector<TileRef>(MAX_SIZE / 2, cache);
    }
}

TileStack* TilePool::closest(TileRef t, double distance)
{
    /*
     * Search for the closest tile set for the provided tile image.
     * Returns the tile stack, if any was found which meets the tile's
     * stated maximum MSE requirement.
     */

    const double epsilon = 1e-3;
    TileStack *closest = NULL;

    for (std::list<TileStack>::iterator i = stackList.begin(); i != stackList.end(); i++) {
        double err = i->median()->errorMetric(*t, distance);

        if (err <= distance) {
            distance = err;
            closest = &*i;

            if (distance < epsilon) {
                // Not going to improve on this; early out.
                break;
            }
        }
    }

    return closest;
}

TileGrid::TileGrid(TilePool *pool)
    : mPool(pool), mWidth(0), mHeight(0)
    {}

void TileGrid::load(const TileOptions &opt, uint8_t *rgba,
                    size_t stride, unsigned width, unsigned height)
{
    mWidth = width / Tile::SIZE;
    mHeight = height / Tile::SIZE;
    tiles.resize(mWidth * mHeight);

    for (unsigned y = 0; y < mHeight; y++)
        for (unsigned x = 0; x < mWidth; x++) {
            TileRef t = Tile::instance(opt,
                                       rgba + (x * Tile::SIZE * 4) +
                                       (y * Tile::SIZE * stride),
                                       stride);
            tiles[x + y * mWidth] = mPool->add(t);
        }
}

void TilePool::optimize(Logger &log)
{
    /*
     * Global optimizations to apply after filling a tile pool.
     */

    if (numFixed) {
        optimizeFixedTiles(log);
    } else {
        optimizePalette(log);
        optimizeTiles(log);
        optimizeTrueColorTiles(log);
        optimizeOrder(log);
    }
}

void TilePool::optimizePalette(Logger &log)
{
    /*
     * Palette optimization, for use after add() but before optimizeTiles().
     *
     * This operates on all tile images in 'tiles', replacing each
     * with a reduced-color version created using a global color
     * palette optimization process.
     */

    ColorReducer reducer;
    bool needReduction = false;
    const double epsilon = 1e-3;

    // First, add ALL tile data to the reducer's pool    
    for (std::vector<TileRef>::iterator i = tiles.begin(); i != tiles.end(); i++) {
        double maxMSE = (*i)->options().getMaxMSE();
        
        if (maxMSE > epsilon) {
            // Lossy mode
            
            needReduction = true;
            for (unsigned j = 0; j < Tile::PIXELS; j++)
                reducer.add((*i)->pixel(j), maxMSE);
        }
    }

    // Skip the rest if all tiles are lossless
    if (needReduction) {
 
        // Ask the reducer to do its own (slow!) global optimization
        reducer.reduce(&log);

        // Now reduce each tile, using the agreed-upon color palette
        for (std::vector<TileRef>::iterator i = tiles.begin(); i != tiles.end(); i++) {
            double maxMSE = (*i)->options().getMaxMSE();        
            if (maxMSE > epsilon)
                *i = (*i)->reduce(reducer);
        }
    }
}

void TilePool::optimizeFixedTiles(Logger &log)
{
    /*
     * This is an alternative optimization path for pools which contain
     * 'fixed' tiles. This means that "numFixed" tiles at the beginning
     * of the tiles[] vector are set in stone, and all other tiles
     * must be removed and replaced with references to these fixed tiles.
     *
     * This is used to implement groups with the 'atlas' parameter.
     */

    /* 
     * All fixed tiles go, in order, into the final data structures.
     */

    stackList.clear();
    stackIndex.resize(numFixed);
    stackArray.resize(numFixed);

    for (unsigned i = 0; i < numFixed; ++i) {
        stackList.push_back(TileStack());
        TileStack *c = &stackList.back();
        c->add(tiles[i]);
        c->index = i;
        stackArray[i] = c;
        stackIndex[i] = c;
    }

    /*
     * All remaining tile serials use closest() to find matches in the fixed stacks.
     */

    log.taskBegin("Matching fixed tiles");

    for (unsigned serial = numFixed; serial < tiles.size(); ++serial) {

        /*
         * We have no specific upper limit on the error, so we could
         * just start out by calling closest() with a distance of HUGE_VAL,
         * but this breaks a lot of the early-out optimizations inside.
         * It's more efficient if we increase the distance gradually.
         */

        double distance = 1.0f;
        TileStack *c;
        do {
            c = closest(tiles[serial], distance);
            distance *= 100;
        } while (!c);

        tiles[serial] = c->median();
        stackIndex.push_back(c);

        if (serial == tiles.size() - 1 || !(serial % 32)) {
            log.taskProgress("%u of %u", serial - numFixed + 1, tiles.size() - numFixed);
        }
    }

    log.taskEnd();
}

void TilePool::optimizeTiles(Logger &log)
{
    /*
     * The tile optimizer is a greedy algorithm that works in two
     * passes: First, we collect tiles into stacks, using an MSE
     * comparison based on the stack's *current* median at the
     * time. This produces a good estimate of the stacks we'll need,
     * but there is no guarantee that the stack we place a tile in
     * will be the most optimal stack for that tile, or that each
     * stack median turns out to be unique.
     *
     * After this collection pass, we run a second pass which does not
     * modify the stored medians, but just chooses the best stack for
     * each input tile. In this stage, it is possible that not every
     * input stack will be used.
     *
     * Collect individual tiles, from "tiles", into TileStacks, stored
     * in 'stackList' and 'stackIndex'.
     */

    std::tr1::unordered_set<TileStack *> activeStacks;

    stackList.clear();
    stackIndex.clear();
    stackIndex.resize(tiles.size());

    log.taskBegin("Gathering pinned tiles");
    optimizeTilesPass(log, activeStacks, true, true);
    log.taskEnd();

    log.taskBegin("Gathering unpinned tiles");
    optimizeTilesPass(log, activeStacks, true, false);
    log.taskEnd();

    log.taskBegin("Optimizing tiles");
    optimizeTilesPass(log, activeStacks, false, false);
    log.taskEnd();
}
    
void TilePool::optimizeTilesPass(Logger &log,
                                 std::tr1::unordered_set<TileStack *> &activeStacks,
                                 bool gather, bool pinned)
{
    // A single pass from the multi-pass optimizeTiles() algorithm

    std::tr1::unordered_map<Tile *, TileStack *> memo;
    
    for (Serial serial = 0; serial < tiles.size(); serial++) {
        TileRef tr = tiles[serial];

        if (tr->options().pinned == pinned) {
            TileStack *c = NULL;

            if (!pinned) {
                /*
                 * Assuming we aren't gathering pinned tiles, we start by looking for
                 * the closest existing stack. This is a very slow step, but we can
                 * save a lot of time on common datasets (with plenty of duplicated tiles)
                 * by memoizing the results.
                 *
                 * This memoization is easy because tiles are unique and immutable.
                 * We only keep the memo for the duration of one optimization pass; the whole
                 * point of a multi-pass algorithm is that we expect the result of closest()
                 * to change between the gathering and optimization passes.
                 *
                 * Note that this memoization does actually change the algorithmic complexity
                 * of our optimization passes. Instead of doing what may be an O(N^2) algorithm
                 * on each pass, this is significantly faster (up to O(N) in the best case). We
                 * end up deferring much more of the optimization to the second pass, whereas
                 * without memoization we end up getting very close to a solution in one pass,
                 * at the expense of quite a lot of performance.
                 *
                 * So in our case, two passes can be much faster than one. Yay.
                 */

                std::tr1::unordered_map<Tile *, TileStack *>::iterator i = memo.find(&*tr);
                if (i == memo.end()) {
                    c = closest(tr, tr->options().getMaxMSE());
                    memo[&*tr] = c;
                } else {
                    c = memo[&*tr];
                }
            }           

            if (!c) {
                // Need to create a fresh stack
                stackList.push_back(TileStack());
                c = &stackList.back();
                c->add(tr);
            } else if (gather) {
                // Add to an existing stack
                c->add(tr);
            }

            if (!gather || pinned) {
                // Remember this stack, we've selected it for good.
                
                assert(c != NULL);
                stackIndex[serial] = c; 
                activeStacks.insert(c);
            }
        }

        if (serial == tiles.size() - 1 || !(serial % 128)) {
            unsigned stacks = gather ? stackList.size() : activeStacks.size();
            log.taskProgress("%u stacks (%.03f%% of total)", stacks,
                             stacks * 100.0 / tiles.size());
        }
    }

    if (!gather) {
        // Permanently delete unused stacks

        std::list<TileStack>::iterator i = stackList.begin();

        while (i != stackList.end()) {
            std::list<TileStack>::iterator j = i;
            i++;

            if (!activeStacks.count(&*j))
                stackList.erase(j);
        }
    }
}

void TilePool::optimizeTrueColorTiles(Logger &log)
{
    /*
     * Look at just the remaining tiles which have too many colors
     * to palettize, and see if we can squeeze them a bit more until they
     * fit in 16 colors.
     *
     * This is a purely *local* palette optimization, where we focus on
     * individual problem tiles. The earlier global palette optimization is
     * more likely to result in uniform color tones across an entire
     * asset group (and avoiding tile discontinuities), whereas this is
     * intended more for tiles that already use a bunch of colors.
     */

    if (stackList.empty())
        return;

    unsigned totalCount = 0;
    unsigned reducedCount = 0;
    log.taskBegin("Optimizing true color tiles");

    std::list<TileStack>::iterator i = stackList.begin();
    while (1) {
        TileStack &stack = *i;
        TileRef tile = stack.median();
        bool isTrueColor = !tile->palette().hasLUT();

        if (isTrueColor) {
            totalCount++;

            // Don't modify tiles that are marked as lossless
            const double epsilon = 1e-3;
            double maxMSE = tile->options().getMaxMSE();
            if (maxMSE > epsilon) {

                /*
                 * Use an unlimited MSE but bounded number of colors, to forcibly
                 * limit this tile to the maximum LUT size (16 colors) without
                 * regard to quality settings.
                 */

                ColorReducer reducer;
                for (unsigned j = 0; j < Tile::PIXELS; j++)
                    reducer.add(tile->pixel(j));
                reducer.reduce(0, TilePalette::LUT_MAX);
                TileRef reduced = tile->reduce(reducer);

                /*
                 * Check the results, and decide whether they're adequate
                 * according to the tile's compression quality.
                 */

                // Try extra hard to avoid CM_TRUE
                double limit = maxMSE * 2.0;

                if (tile->errorMetric(*reduced, limit) < limit) {
                    stack.replace(reduced);
                    reducedCount++;
                }
            }
        }

        ++i;
        bool last = i == stackList.end();

        // Update status periodically as well as on completion.
        if (last || (isTrueColor && (totalCount % 16) == 0)) {
            log.taskProgress("%u of %u tile%s reduced (%.03f%%)",
                reducedCount, totalCount,
                totalCount == 1 ? "" : "s",
                totalCount ? (reducedCount * 100.0 / totalCount) : 0);
        }

        if (last)
            break;
    }

    log.taskEnd();
}

void TilePool::optimizeOrder(Logger &log)
{
    /*
     * Optimize the order of our tiles within the pool. It turns out
     * that this is a very computationally difficult problem- it's
     * actually an asymmetric travelling salesman problem!
     *
     * It isn't even remotely computationally feasible to come up with
     * a globally optimal solution, so we just use a greedy heuristic
     * which tries to pick the best next tile. This uses
     * TileCodecLUT::encode() as a cost metric.
     *
     * This is where we assign an index to each stack, and build the
     * stackArray.
     */

    std::list<TileStack> newOrder;
    TileCodecLUT codec;
    unsigned totalCost = 0;
    bool pinned = true;

    log.taskBegin("Optimizing tile order");

    stackArray.clear();

    while (!stackList.empty()) {
        std::list<TileStack>::iterator chosen;

        if (pinned && stackList.front().isPinned()) {
            /*
             * We found a consecutive pair of pinned tiles. We're obligated to maintain
             * the ordering of these tiles, so there's no opportunity for optimization.
             */
            
            chosen = stackList.begin();

        } else {
            /*
             * Pick the lowest-cost tile next. Use a forked copy of
             * the codec to ask what the cost would be for each
             * possible choice.
             */

            unsigned bestCost = (unsigned) -1;

            for (std::list<TileStack>::iterator i = stackList.begin(); i != stackList.end(); i++) {
                TileCodecLUT codecFork = codec;
                unsigned cost = codecFork.encode(i->median()->palette());
                if (cost < bestCost) {
                    bestCost = cost;
                    chosen = i;
                }
            }
        }

        // Apply the new codec state permanently
        totalCost += codec.encode(chosen->median()->palette());

        // Pick the best tile, and assign it a permanent index
        pinned = chosen->isPinned();
        chosen->index = stackArray.size();
        stackArray.push_back(&*chosen);
        newOrder.splice(newOrder.end(), stackList, chosen);

        if (!(stackList.size() % 128))
            log.taskProgress("%d tiles (cost %d)",
                             (int) newOrder.size(), totalCost);
    }

    stackList.swap(newOrder);

    log.taskEnd();
}

void TilePool::encode(std::vector<uint8_t>& out, Logger *log)
{
    TileCodec codec(out);

    if (log) {
        log->taskBegin("Encoding tiles");
        log->taskProgress("%d tiles (%d bytes in flash)",
            stackList.size(), stackList.size() * FlashAddress::TILE_SIZE);
    }

    for (std::list<TileStack>::iterator i = stackList.begin(); i != stackList.end(); i++)
        codec.encode(i->median());
    codec.flush();

    if (log) {
        log->taskEnd();
        codec.dumpStatistics(*log);
    }
}

void TilePool::calculateCRC(std::vector<uint8_t> &crcbuf) const
{
    /*
     * Pre-calculate the CRC for all tiles in this Pool.
     *
     * The base can ask the cube to compute this at runtime, and compare to determine
     * whether the its record of what's installed on the cube is still valid.
     */

    static const uint8_t gf84[] = {
        // Lookup table for Galois Field multiplication by our chosen polynomial, 0x84.
        0x00, 0x84, 0x13, 0x97, 0x26, 0xa2, 0x35, 0xb1, 0x4c, 0xc8, 0x5f, 0xdb, 0x6a, 0xee, 0x79, 0xfd,
        0x98, 0x1c, 0x8b, 0x0f, 0xbe, 0x3a, 0xad, 0x29, 0xd4, 0x50, 0xc7, 0x43, 0xf2, 0x76, 0xe1, 0x65,
        0x2b, 0xaf, 0x38, 0xbc, 0x0d, 0x89, 0x1e, 0x9a, 0x67, 0xe3, 0x74, 0xf0, 0x41, 0xc5, 0x52, 0xd6,
        0xb3, 0x37, 0xa0, 0x24, 0x95, 0x11, 0x86, 0x02, 0xff, 0x7b, 0xec, 0x68, 0xd9, 0x5d, 0xca, 0x4e,
        0x56, 0xd2, 0x45, 0xc1, 0x70, 0xf4, 0x63, 0xe7, 0x1a, 0x9e, 0x09, 0x8d, 0x3c, 0xb8, 0x2f, 0xab,
        0xce, 0x4a, 0xdd, 0x59, 0xe8, 0x6c, 0xfb, 0x7f, 0x82, 0x06, 0x91, 0x15, 0xa4, 0x20, 0xb7, 0x33,
        0x7d, 0xf9, 0x6e, 0xea, 0x5b, 0xdf, 0x48, 0xcc, 0x31, 0xb5, 0x22, 0xa6, 0x17, 0x93, 0x04, 0x80,
        0xe5, 0x61, 0xf6, 0x72, 0xc3, 0x47, 0xd0, 0x54, 0xa9, 0x2d, 0xba, 0x3e, 0x8f, 0x0b, 0x9c, 0x18,
        0xac, 0x28, 0xbf, 0x3b, 0x8a, 0x0e, 0x99, 0x1d, 0xe0, 0x64, 0xf3, 0x77, 0xc6, 0x42, 0xd5, 0x51,
        0x34, 0xb0, 0x27, 0xa3, 0x12, 0x96, 0x01, 0x85, 0x78, 0xfc, 0x6b, 0xef, 0x5e, 0xda, 0x4d, 0xc9,
        0x87, 0x03, 0x94, 0x10, 0xa1, 0x25, 0xb2, 0x36, 0xcb, 0x4f, 0xd8, 0x5c, 0xed, 0x69, 0xfe, 0x7a,
        0x1f, 0x9b, 0x0c, 0x88, 0x39, 0xbd, 0x2a, 0xae, 0x53, 0xd7, 0x40, 0xc4, 0x75, 0xf1, 0x66, 0xe2,
        0xfa, 0x7e, 0xe9, 0x6d, 0xdc, 0x58, 0xcf, 0x4b, 0xb6, 0x32, 0xa5, 0x21, 0x90, 0x14, 0x83, 0x07,
        0x62, 0xe6, 0x71, 0xf5, 0x44, 0xc0, 0x57, 0xd3, 0x2e, 0xaa, 0x3d, 0xb9, 0x08, 0x8c, 0x1b, 0x9f,
        0xd1, 0x55, 0xc2, 0x46, 0xf7, 0x73, 0xe4, 0x60, 0x9d, 0x19, 0x8e, 0x0a, 0xbb, 0x3f, 0xa8, 0x2c,
        0x49, 0xcd, 0x5a, 0xde, 0x6f, 0xeb, 0x7c, 0xf8, 0x05, 0x81, 0x16, 0x92, 0x23, 0xa7, 0x30, 0xb4
    };

    const unsigned FLASH_CRC_LEN = 16;
    const unsigned TILES_PER_BLOCK = 16;

    crcbuf.resize(FLASH_CRC_LEN);
    memset(&crcbuf.front(), 0, FLASH_CRC_LEN);

    unsigned addr = 0;
    unsigned numBlocks = (size() + (TILES_PER_BLOCK - 1)) / TILES_PER_BLOCK;

    for (unsigned i = 0; i < numBlocks; ++i) {

        uint8_t crc = 0xff;
        addr &= ~0x7f;

        for (unsigned tile = 0; tile < TILES_PER_BLOCK; ++tile) {
            for (unsigned sample = 0; sample < 4; ++sample) {
                crc = gf84[crc] ^ rawByte(addr);
                addr = (addr & ~0x7f) | (crc >> 1);
            }

            crcbuf[tile] ^= crc;
            addr += 0x80;
        }
    }
}


};  // namespace Stir
