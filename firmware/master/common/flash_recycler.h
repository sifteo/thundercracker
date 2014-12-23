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

/*
 * The Block Recycler is part of the Volume layer. This is how we gather
 * fresh erased blocks for constructing new volumes. These erased blocks
 * can come from several sources:
 *
 *   - The erase log
 *   - Orphaned blocks
 *   - Deleted or incomplete volumes
 *
 * This module is ultimately responsible for wear-leveling, as it decides
 * what order to recycle blocks in.
 */

#ifndef FLASH_RECYCLER_H_
#define FLASH_RECYCLER_H_

#include "macros.h"
#include "flash_map.h"
#include "flash_eraselog.h"


/**
 * Manages the process of finding unused FlashMapBlocks to recycle.
 * Typically we look for blocks in deleted volumes, starting with
 * the lowest-erase-count blocks that aren't part of a volume header.
 *
 * We use header blocks only when all other blocks in that volume have
 * already been recycled, so that we don't lose any erase count data.
 *
 * If we spot any orphaned blocks (not part of any volume, even a deleted
 * one) we unfortunately have no way of knowing the true erase count of
 * those blocks. We prefer to use these orphaned blocks before recycling
 * deleted blocks, since we make up an erase count by taking the average
 * of all known erase counts. If we didn't use the orphaned blocks first,
 * the calculated pseudo-erase-count would increase over time, causing
 * these orphan blocks to systematically appear more heavily worn than
 * they actually are, causing us to overwear the non-orphaned blocks.
 *
 * This property of the recycler is especially important when dealing with
 * a blank or heavily damaged filesystem, in which large numbers of blocks
 * (possibly all of them) are orphaned.
 *
 * Much of the complexity here comes from trying to keep the memory usage
 * and algorithmic complexity low. For example, we don't want to spend the
 * memory on a full table of blocks sorted by erase count, nor do we want
 * to re-scan the device once per recycled block. It turns out that it isn't
 * actually important that we pick the absolute lowest-erase-count block
 * every time. As long as we tend to pick lower-erase-count blocks first,
 * and we accurately propagate erase counts, we'll still provide wear
 * leveling.
 *
 * Since we prefer to operate within a single deleted volume at a time,
 * our approach involves keeping a set of candidate volumes in which at least
 * one of their blocks has an erase count <= the average. This candidate list
 * is always preferred when searching for blocks to recycle.
 */

class FlashBlockRecycler {
public:
    typedef uint32_t EraseCount;

    FlashBlockRecycler(bool useEraseLog=true);

    /**
     * Find the next recyclable block, as well as its erase count.
     *
     * There are multiple sources for recyclable blocks. This class searches
     * all of them, in a predefined order. Most recyclable blocks must be
     * erased first. We have a special cache of pre-erased blocks (the 'erase log')
     * which is optional. By default, it is consulted first.
     *
     * The returned block is guaranteed to be erased.
     */
    bool next(FlashMapBlock &block, EraseCount &eraseCount);

private:
    FlashMapBlock::Set orphanBlocks;            // Not reachable from anywhere
    FlashMapBlock::Set deletedVolumes;          // Header blocks for deleted volumes
    FlashMapBlock::Set eraseLogVolumes;         // Blocks used to store the erase log
    FlashMapBlock::Set candidateVolumes;        // Current list of recycling candidates
    uint32_t averageEraseCount;
    bool useEraseLog;

    FlashEraseLog eraseLog;
    FlashBlockWriter dirtyVolume;

    void findOrphansAndDeletedVolumes();
    void findCandidateVolumes();
};


#endif
