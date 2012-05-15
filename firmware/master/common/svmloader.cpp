/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "svmruntime.h"
#include "svmloader.h"
#include "elfprogram.h"
#include "flash_blockcache.h"
#include "flash_volume.h"
#include "svm.h"
#include "svmmemory.h"
#include "svmdebugpipe.h"
#include "radio.h"
#include "tasks.h"
#include "panic.h"
#include "cubeslots.h"

#include <sifteo/abi.h>
#include <string.h>
#include <stdlib.h>

using namespace Svm;


void SvmLoader::logTitleInfo(const Elf::Program &program)
{
#ifdef SIFTEO_SIMULATOR
    FlashBlockRef ref;

    const char *title = program.getMetaString(ref, _SYS_METADATA_TITLE_STR);
    LOG(("SVM: Preparing to run title \"%s\"\n", title ? title : "(untitled)"));

    const _SYSUUID *uuid = program.getMetaValue<_SYSUUID>(ref, _SYS_METADATA_UUID);
    if (uuid) {
        LOG(("SVM: Title UUID is {"));
        for (unsigned i = 0; i < 16; i++)
            LOG(("%02x%s", uuid->bytes[i], (i == 3 || i == 5 || i == 7 || i == 9) ? "-" : ""));
        LOG(("}\n"));
    }
#endif
}

_SYSCubeIDVector SvmLoader::getCubeVector(const Elf::Program &program)
{
    /*
     * XXX: BIG HACK.
     *
     * Temporary code to initialize cubes, using the CubeRange from
     * a game's metadata.
     */

    // Look up CubeRange
    FlashBlockRef ref;
    const _SYSMetadataCubeRange *range =
        program.getMetaValue<_SYSMetadataCubeRange>(ref, _SYS_METADATA_CUBE_RANGE);
    unsigned minCubes = range ? range->minCubes : 0;

    if (!minCubes) {
        LOG(("SVM: No CubeRange found, not initializing any cubes.\n"));
        return 0;
    }

    _SYSCubeIDVector cubes = 0xFFFFFFFF << (32 - minCubes);

    return cubes;
}

void SvmLoader::bootstrapAssets(const Elf::Program &program, _SYSCubeIDVector cubes)
{
    /*
     * XXX: BIG HACK.
     *
     * Temporary code to load the bootstrap assets specified
     * in a game's metadata. Normally this would be handled by
     * the system menu.
     *
     * In a real loader scenario, bootstrap assets would be loaded
     * onto a set of cubes determined by the loader: likely calculated
     * by looking at the game's supported cube range and the number of
     * connected cubes.
     *
     * In this hack, we just load the bootstrap assets onto the
     * first N cubes, where N is the minimum number required by the game.
     * If no cube range was specified, we don't load assets at all.
     */

    // Look up BootAsset array
    uint32_t actualSize;
    FlashBlockRef ref;
    _SYSMetadataBootAsset *vec = (_SYSMetadataBootAsset*)
        program.getMeta(ref, _SYS_METADATA_BOOT_ASSET, sizeof *vec, actualSize);
    if (!vec) {
        LOG(("SVM: No bootstrap assets found\n"));
        return;
    }
    unsigned count = actualSize / sizeof *vec;

    if (!cubes) {
        LOG(("SVM: Not loading bootstrap assets, no CubeRange found\n"));
        return;
    }

#if defined(SIFTEO_SIMULATOR) && !defined(ASSET_BOOTSTRAP_SLOW_AND_STEADY)
    CubeSlots::simAssetLoaderBypass = true;
#endif

    for (unsigned i = 0; i < count; i++) {
        _SYSMetadataBootAsset &BA = vec[i];
        PanicMessenger msg;

        // Allocate some things in user RAM.
        const SvmMemory::VirtAddr loaderVA = 0x10000;
        const SvmMemory::VirtAddr groupVA = 0x11000;
        msg.init(0x12000);

        SvmMemory::PhysAddr loaderPA;
        SvmMemory::PhysAddr groupPA;
        SvmMemory::mapRAM(loaderVA, 1, loaderPA);
        SvmMemory::mapRAM(groupVA, 1, groupPA);

        _SYSAssetLoader *loader = reinterpret_cast<_SYSAssetLoader*>(loaderPA);
        _SYSAssetGroup *group = reinterpret_cast<_SYSAssetGroup*>(groupPA);
        _SYSAssetLoaderCube *lc = reinterpret_cast<_SYSAssetLoaderCube*>(loader + 1);

        loader->cubeVec = 0;
        group->pHdr = BA.pHdr;

        if (_SYS_asset_findInCache(group, cubes) == cubes) {
            LOG(("SVM: Bootstrap asset group %s already installed\n",
                SvmDebugPipe::formatAddress(BA.pHdr).c_str()));
            continue;
        }

        LOG(("SVM: Installing bootstrap asset group %s in slot %d\n",
            SvmDebugPipe::formatAddress(BA.pHdr).c_str(), BA.slot));

        if (!_SYS_asset_loadStart(loader, group, BA.slot, cubes)) {
            // Out of space. Erase the slot first.
            LOG(("SVM: Erasing asset slot\n"));
            _SYS_asset_slotErase(BA.slot);
            _SYS_asset_loadStart(loader, group, BA.slot, cubes);
        }

        if ((loader->complete & cubes) != cubes)
            for (;;) {

                // Draw status to each cube
                _SYSCubeIDVector statusCV = cubes;
                while (statusCV) {
                    _SYSCubeID c = Intrinsic::CLZ(statusCV);
                    statusCV ^= Intrinsic::LZ(c);

                    msg.at(1,1) << "Bootstrapping";
                    msg.at(1,2) << "game assets...";
                    msg.at(4,5) << lc[c].progress;
                    msg.at(7,7) << "of";
                    msg.at(4,9) << lc[c].dataSize;

                    msg.paint(c);
                }
            
                // Are we done? Leave with the final status on-screen
                if ((loader->complete & cubes) == cubes)
                    break;

                // Load for a while, with the display idle. The PanicMessenger
                // is really wasteful with the cube's CPU time, so we need to
                // paint pretty infrequently in order to load assets full-speed.

                uint32_t milestone = lc[0].progress + 2000;
                while (lc[0].progress < milestone 
                       && (loader->complete & cubes) != cubes) {
                    Tasks::work();
                    Radio::halt();
                }
            }

        _SYS_asset_loadFinish(loader);

        LOG(("SVM: Finished instaling bootstrap asset group %s\n",
            SvmDebugPipe::formatAddress(BA.pHdr).c_str()));
    }

#ifdef SIFTEO_SIMULATOR
    CubeSlots::simAssetLoaderBypass = false;
#endif
}

void SvmLoader::bootstrap(const Elf::Program &program)
{
    // Use the game's RODATA segment
    SvmMemory::setFlashSegment(0, program.getRODataSpan());

    // Enable the game's minimum set of cubes
    _SYSCubeIDVector cv = getCubeVector(program);
    _SYS_enableCubes(cv);

    // Temporary asset bootstrapper
    bootstrapAssets(program, cv);

    // PanicMessenger leaves CubeSlot out of sync with the cube's paint state.
    // Reset all CubeSlot state before running the game.
    _SYS_disableCubes(cv);
    _SYS_enableCubes(cv);
}

void SvmLoader::loadRWData(const Elf::Program &program)
{
    FlashBlockRef ref;
    const Elf::ProgramHeader *ph = program.getRWDataSegment(ref);
    if (!ph)
        return;

    /*
     * Temporarily use segment 0 to do the copy.
     * If anything is wrong, this will cause an SVM fault, just as if
     * the copy was done from userspace.
     */
    SvmMemory::setFlashSegment(0, program.getProgramSpan());
    _SYS_memcpy8(reinterpret_cast<uint8_t*>( ph->p_vaddr ),
                 reinterpret_cast<uint8_t*>( ph->p_offset + SvmMemory::SEGMENT_0_VA ),
                 ph->p_memsz);
}

void SvmLoader::run(const Elf::Program &program)
{
    // On simulator builds, log some info about the program we're running
    logTitleInfo(program);

    // Reset the debugging and logging subsystem
    SvmDebugPipe::init();

    // On simulation, with the built-in debugger, point SvmDebug to
    // the proper ELF binary to load debug symbols from.
    SvmDebugPipe::setSymbolSource(program);

    // Setup that the loader will eventually be responsible for...
    bootstrap(program);

    // Initialize memory and CPU
    SvmMemory::erase();
    SvmCpu::init();

    // Load RWDATA into RAM
    loadRWData(program);
 
    // Set up default flash segment
    SvmMemory::setFlashSegment(0, program.getRODataSpan());

    SvmRuntime::run(program.getEntry(), program.getTopOfRAM(),
                    SvmMemory::VIRTUAL_RAM_TOP);
}

void SvmLoader::run(int id)
{
    // XXX: Temporary
#if 0

    FlashVolumeIter vi;
    FlashVolume vol;
    while (vi.next(vol)) {
        LOG(("VOLUME: Found volume, type %x\n", vol.getType()));
    }
    
    FlashBlockRecycler br;
    FlashMapBlock block;
    FlashBlockRecycler::EraseCount ec;
    unsigned i = 0;
    while (br.next(block, ec)) {
        LOG(("RECYCLE #%d: %x ec=%d\n", i++, block.address(), ec));
    }

#endif

    // Set up an identity mapping
    FlashMap map;
    for (unsigned i = 0; i < arraysize(map.blocks); i++)
        map.blocks[i].setIndex(i);

    Elf::Program program;
    if (program.init(FlashMapSpan::create(&map, 0, 0xFFFF)))
        run(program);
}


void SvmLoader::exit(bool fault)
{
#ifdef SIFTEO_SIMULATOR
    // Must preserve the error code here, so that unit tests and other scripts work.
    ::exit(fault);
#endif
    while (1);
}
