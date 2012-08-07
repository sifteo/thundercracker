/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "assetutil.h"


_SYSAssetGroupCube *AssetUtil::mapGroupCube(SvmMemory::VirtAddr group, _SYSCubeID cid)
{
	ASSERT(cid < _SYS_NUM_CUBE_SLOTS);

	SvmMemory::VirtAddr va = group + sizeof(_SYSAssetGroup) + cid * sizeof(_SYSAssetGroupCube);
	SvmMemory::PhysAddr pa;

	if (!SvmMemory::mapRAM(va, sizeof(_SYSAssetGroupCube), pa))
		return 0;

	return reinterpret_cast<_SYSAssetGroupCube*>(pa);
}

_SYSAssetLoaderCube *AssetUtil::mapLoaderCube(SvmMemory::VirtAddr loader, _SYSCubeID cid)
{
	ASSERT(cid < _SYS_NUM_CUBE_SLOTS);

	SvmMemory::VirtAddr va = loader + sizeof(_SYSAssetLoader) + cid * sizeof(_SYSAssetLoaderCube);
	SvmMemory::PhysAddr pa;

	if (!SvmMemory::mapRAM(va, sizeof(_SYSAssetLoaderCube), pa))
		return 0;

	return reinterpret_cast<_SYSAssetLoaderCube*>(pa);
}

unsigned AssetUtil::loadedBaseAddr(SvmMemory::VirtAddr group, _SYSCubeID cid)
{
	_SYSAssetGroupCube *gc = mapGroupCube(group, cid);
	return gc ? gc->baseAddr : 0;
}

