/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "svmruntime.h"
#include "svmloader.h"
#include "elfutil.h"
#include "flashlayer.h"
#include "svm.h"
#include "svmmemory.h"
#include "svmdebugpipe.h"
#include "radio.h"
#include "tasks.h"
#include "panic.h"

#include <sifteo/abi.h>
#include <string.h>
#include <stdlib.h>

using namespace Svm;


static void logTitleInfo(Elf::ProgramInfo &pInfo)
{
#ifdef SIFTEO_SIMULATOR
    FlashBlockRef ref;

    const char *title = pInfo.meta.getString(ref, _SYS_METADATA_TITLE_STR);
    LOG(("SVM: Preparing to run title \"%s\"\n", title ? title : "(untitled)"));

    const _SYSUUID *uuid = pInfo.meta.getValue<_SYSUUID>(ref, _SYS_METADATA_UUID);
    if (uuid) {
        LOG(("SVM: Title UUID is {"));
        for (unsigned i = 0; i < 16; i++)
            LOG(("%02x%s", uuid->bytes[i], (i == 3 || i == 5 || i == 7 || i == 9) ? "-" : ""));
        LOG(("}\n"));
    }
#endif
}

static _SYSCubeIDVector xxxInitCubes(Elf::ProgramInfo &pInfo)
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
        pInfo.meta.getValue<_SYSMetadataCubeRange>(ref, _SYS_METADATA_CUBE_RANGE);
    unsigned minCubes = range ? range->minCubes : 0;

    if (!minCubes) {
        LOG(("SVM: No CubeRange found, not initializing any cubes.\n"));
        return 0;
    }

    _SYSCubeIDVector cubes = 0xFFFFFFFF << (32 - minCubes);
    _SYS_enableCubes(cubes);

    return cubes;
}

static void xxxBootstrapAssets(Elf::ProgramInfo &pInfo, _SYSCubeIDVector cubes)
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
        pInfo.meta.get(ref, _SYS_METADATA_BOOT_ASSET, sizeof *vec, actualSize);
    if (!vec) {
        LOG(("SVM: No bootstrap assets found\n"));
        return;
    }
    unsigned count = actualSize / sizeof *vec;

    if (!cubes) {
        LOG(("SVM: Not loading bootstrap assets, no CubeRange found\n"));
        return;
    }

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
}


void SvmLoader::run(uint16_t appId)
{
    // TODO: look this up via appId
    FlashRange elf(0, 0xFFFF0000);

    Elf::ProgramInfo pInfo;
    if (!pInfo.init(elf))
        return;

    // On simulator builds, log some info about the program we're running
    logTitleInfo(pInfo);

    // On simulation, with the built-in debugger, point SvmDebug to
    // the proper ELF binary to load debug symbols from.
    SvmDebugPipe::setSymbolSourceELF(elf);

    // Initialize rodata segment
    SvmMemory::setFlashSegment(pInfo.rodata.data);

    // Setup that the loader will eventually be responsible for...
    xxxBootstrapAssets(pInfo, xxxInitCubes(pInfo));

    // Clear RAM (including implied BSS)
    SvmMemory::erase();
    SvmCpu::init();

    // Load RWDATA into RAM
    SvmMemory::PhysAddr rwdataPA;
    if (!pInfo.rwdata.data.isEmpty() &&
        SvmMemory::mapRAM(SvmMemory::VirtAddr(pInfo.rwdata.vaddr),
            pInfo.rwdata.size, rwdataPA)) {
        FlashStream rwDataStream(pInfo.rwdata.data);
        rwDataStream.read(rwdataPA, pInfo.rwdata.size);
    }

    SvmRuntime::run(pInfo.entry,
                    pInfo.bss.vaddr + pInfo.bss.size,
                    SvmMemory::VIRTUAL_RAM_TOP);
}

void SvmLoader::exit()
{
#ifdef SIFTEO_SIMULATOR
    ::exit(0);
#endif
    while (1);
}
