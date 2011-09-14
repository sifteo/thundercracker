/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "cube.h"

CubeSlot CubeSlot::instances[_SYS_NUM_CUBE_SLOTS];
_SYSCubeIDVector CubeSlot::vecEnabled;


void CubeSlot::loadAssets(_SYSAssetGroup *a) {
    if (isAssetGroupLoaded(a))
	return;

    // XXX: Pick a base address too!
    a->cubes[id()].progress = 0;

    a->reqCubes |= bit();
    loadGroup = a;
}

bool CubeSlot::radioProduce(PacketTransmission &tx)
{
    if (vbuf->cm4) {
	/* Video buffer updates */
    }

    if (loadGroup) {
	/* Asset downloading */
    }

    return false;
}

void CubeSlot::radioAcknowledge(const PacketBuffer &packet)
{
}

void CubeSlot::radioTimeout()
{
}
