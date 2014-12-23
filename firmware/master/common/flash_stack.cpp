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

#include "macros.h"
#include "flash_stack.h"
#include "flash_blockcache.h"
#include "flash_lfs.h"
#include "flash_syslfs.h"
#include "flash_eraselog.h"
#include "flash_recycler.h"
#include "svmloader.h"
#include "tasks.h"

void FlashStack::init()
{
    FlashDevice::init();
    FlashBlock::init();
    FlashLFSCache::invalidate();
}


void FlashStack::invalidateCache(unsigned flags)
{
    FlashBlock::invalidate(flags);
    FlashLFSCache::invalidate();
}


void FlashStack::reformatDevice()
{
    /*
     * Do a physical wipe of the flash device.
     *
     * Bonus: since we now know everything is erased, we can mark all blocks as
     * pre-erased, allowing subsequent write operations to avoid erasing
     * what would otherwise be considered "orphaned" blocks.
     */

    FlashDevice::eraseAll();

    // max timeout for chip erase is 200 seconds (!)
    SysTime::Ticks timeout = SysTime::ticks() + SysTime::sTicks(200);
    while (FlashDevice::busy()) {
        if (SysTime::ticks() < timeout)
            Tasks::resetWatchdog();
    }

    /*
     * Invalidate all caches. Presumably at least one code page will still be
     * referenced, if we're executing code via SVM. This fills those referenced
     * pages with an "abort trap" pattern, which wedges the virtual CPU in a
     * _SYS_abort() loop until we have something better for it to do..
     */

    invalidateCache(FlashBlock::F_ABORT_TRAP);

    /*
     * Log the erased blocks only after we're sure they have finished erasing
     */

    FlashBlockRecycler recycler;
    FlashEraseLog log;

    for (unsigned i = 0; i < FlashMapBlock::NUM_BLOCKS; ++i) {

        // Must allocate before checking log.currentVolume below!
        if (!log.allocate(recycler))
            break;

        /*
         * We're writing our erase log data to a block, which means it's
         * no longer erased- ensure we don't represent it as such.
         */
        FlashMapBlock block = FlashMapBlock::fromIndex(i);
        if (log.currentVolume().block.code == block.code)
            continue;

        // Initial erase count is 1
        FlashEraseLog::Record rec = { 1, block };
        log.commit(rec);
        Tasks::resetWatchdog();
    }

    SysLFS::invalidateClients();
}
