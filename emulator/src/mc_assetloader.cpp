/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <protocol.h>
#include "assetloader.h"
#include "assetutil.h"
#include "cube.h"
#include "cubeslots.h"
#include "lsdec.h"
#include "system.h"
#include "system_mc.h"
#include "svmdebugpipe.h"

bool AssetLoader::simBypass;


bool AssetLoader::loaderBypass(_SYSCubeID id, AssetGroupInfo &group)
{
    /*
     * This function implements "Asset Loader Bypass" mode, which is in effect
     * any time simBypass is True. If we can load an asset group ourselves, 
     * this function can return true and the real asset loader will not be
     * invoked for this group.
     *
     * We use our reference LoadstreamDecoder, a C++ implementation of the
     * same decompressor that our cube firmware uses. We can decompress the
     * asset data nearly instantly, and write the results directly to simulated
     * cube flash memory.
     */

    if (!simBypass)
        return false;

    /*
     * Prep for loading
     */

    _SYSAssetGroupCube *agc = AssetUtil::mapGroupCube(group.va, id);
    if (!agc)
        return false;
    unsigned baseAddr = agc->baseAddr;

    Cube::Hardware *simCube = SystemMC::getCubeForSlot(&CubeSlots::instances[id]);
    if (!simCube)
        return false;

    FlashStorage::CubeRecord *storage = simCube->flash.getStorage();
    LoadstreamDecoder lsdec(storage->ext, sizeof storage->ext);

    lsdec.setAddress(baseAddr << 7);

    /*
     * Now we just have to feed bytes into lsdec, from the asset
     * group data just after group.headerVA. This isn't quite trivial,
     * since the header VA is interpreted differently depending on
     * whether group.remapToVolume is set.
     *
     * So, to reuse as much non-simulator code as possible, we'll
     * use AssetFIFO to read this data out of our AssetGroupInfo,
     * then we'll shuttle data from that FIFO to lsdec.
     */

    uint32_t offset = 0;
    _SYSAssetLoaderCube buffer;
    memset(&buffer, 0, sizeof buffer);

    while (offset != group.dataSize) {
        offset += AssetFIFO::fetchFromGroup(buffer, group, offset);

        AssetFIFO fifo(buffer);
        while (fifo.readAvailable())
            lsdec.handleByte(fifo.read());
        fifo.commitReads();
    }

    LOG(("ASSET[%d]: Installed asset group %s at base address "
        "0x%08x (loader bypassed)\n",
        id, SvmDebugPipe::formatAddress(group.headerVA).c_str(), baseAddr));

    return true;
}
