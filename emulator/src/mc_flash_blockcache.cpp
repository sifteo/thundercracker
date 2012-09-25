/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash_blockcache.h"
#include "svmdebugpipe.h"
#include "svmmemory.h"
#include "system.h"
#include "system_mc.h"
#include <vector>
#include <algorithm>

FlashBlock::FlashStats FlashBlock::stats;


bool FlashBlock::isAddrValid(uintptr_t pa)
{
    // Quick predicate to check a physical address. Used only in simulation.

    uintptr_t offset = reinterpret_cast<uint8_t*>(pa) - &mem[0][0];
    return offset < sizeof mem;
}

void FlashBlock::verify()
{
    FlashDevice::verify(address, getData(), BLOCK_SIZE);
}

void FlashBlock::resetStats()
{
    memset(&stats.periodic, 0, sizeof stats.periodic);
}

void FlashBlock::countBlockMiss(uint32_t blockAddr)
{
    stats.periodic.blockMiss++;

    unsigned blockNumber = blockAddr / BLOCK_SIZE;
    ASSERT(blockNumber < arraysize(stats.periodic.blockMissCounts));
    stats.periodic.blockMissCounts[blockNumber]++;
}

bool FlashBlock::hotBlockSort(unsigned i, unsigned j) {
    return stats.periodic.blockMissCounts[j] < stats.periodic.blockMissCounts[i];
}

void FlashBlock::dumpStats()
{
    const SysTime::Ticks interval = SysTime::sTicks(1);
    const double flashBusMHZ = 18.0;
    const double bytesToMBits = 10.0 * 1e-6;
    const unsigned numHotBlocks = 10;

    if (!SystemMC::getSystem()->opt_svmFlashStats)
        return;
    
    SysTime::Ticks now = SysTime::ticks();
    SysTime::Ticks tickDiff = now - stats.timestamp;
    if (tickDiff < interval)
        return;

    double dt = tickDiff / (double) SysTime::sTicks(1);
    uint32_t totalBytes = stats.periodic.blockMiss * BLOCK_SIZE;
    double effectiveMHZ = totalBytes / dt * bytesToMBits;

    /*
     * Print overall hit/miss stats and simulated bus utilization
     */

    LOG(("\nFLASH: %9.1f acc/s, %8.1f same/s, "
        "%8.1f cached/s, %8.1f miss/s, "
        "%8.2f%% bus utilization\n",
        stats.periodic.blockTotal / dt,
        stats.periodic.blockHitSame / dt,
        stats.periodic.blockHitOther / dt,
        stats.periodic.blockMiss / dt,
        effectiveMHZ / flashBusMHZ * 100.0));

    /*
     * Log the N 'hottest' blocks; those with the most repeated misses.
     */

    std::vector<unsigned> hotBlocks(arraysize(stats.periodic.blockMissCounts));
    for (unsigned i = 0; i < arraysize(hotBlocks); ++i)
        hotBlocks[i] = i;

    std::partial_sort(&hotBlocks[0], &hotBlocks[numHotBlocks],
        &hotBlocks[arraysize(hotBlocks)], hotBlockSort);

    for (unsigned i = 0; i < numHotBlocks; ++i) {
        uint32_t blockNum = hotBlocks[i];
        uint32_t numMisses = stats.periodic.blockMissCounts[blockNum];
        uint32_t blockAddr = blockNum * BLOCK_SIZE;
        SvmMemory::VirtAddr va = SvmMemory::flashToVirtAddr(blockAddr);
        std::string name = SvmDebugPipe::formatAddress(va);

        if (numMisses == 0)
            break;

        LOG(("FLASH: [%5d miss] @ addr=0x%06x va=%08x  %s\n",
            numMisses, blockAddr, (unsigned)va, name.c_str()));
    }

    /*
     * Next stats interval...
     */

    resetStats();
    stats.timestamp = now;
}
