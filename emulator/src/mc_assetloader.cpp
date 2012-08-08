/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <protocol.h>
#include "assetloader.h"

bool AssetLoader::simBypass;


bool AssetLoader::loaderBypass(_SYSCubeID id, AssetGroupInfo &group)
{
    if (!simBypass)
        return false;

    _SYSAssetGroupCube *agc = AssetUtil::mapGroupCube(group.va, id);
    if (!agc)
        return false;
    unsigned baseAddr = agc->baseAddr;

    CubeSlot &cube = CubeSlots::instances[id];
    Cube::Hardware *simCube = SystemMC::getCubeForSlot(cube);
    if (!simCube)
        return false;

    FlashStorage::CubeRecord *storage = simCube->flash.getStorage();
    LoadstreamDecoder lsdec(storage->ext, sizeof storage->ext);

    lsdec.setAddress(baseAddr << 7);
    lsdec.handleSVM(group.headerVA + sizeof header, header.dataSize);

    LOG(("FLASH[%d]: Installed asset group %s at base address "
        "0x%08x (loader bypassed)\n",
        id(), SvmDebugPipe::formatAddress(G->pHdr).c_str(), baseAddr));

        // Mark this as done already.
        LC->progress = header.dataSize;
        Atomic::SetLZ(L->complete, id());

        return;
    }
}
    #endif

