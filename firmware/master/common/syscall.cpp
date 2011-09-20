/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Implementations for all syscall handlers.
 *
 * This is our front line of defense against buggy or malicious game
 * code, so here is where we need to carefully validate all input
 * values. The object implementations past this level don't
 * necessarily validate their input.
 */

#include <sifteo/abi.h>
#include "radio.h"
#include "cube.h"
#include "runtime.h"

extern "C" {

struct _SYSEventVectors _SYS_vectors;


void _SYS_exit(void)
{
    Runtime::exit();
}

void _SYS_yield(void)
{
    Radio::halt();
}

void _SYS_paint(void)
{
    Radio::halt();
}

void _SYS_enableCubes(_SYSCubeIDVector cv)
{
    CubeSlot::enableCubes(CubeSlot::truncateVector(cv));
}

void _SYS_disableCubes(_SYSCubeIDVector cv)
{
    CubeSlot::disableCubes(CubeSlot::truncateVector(cv));
}

void _SYS_setVideoBuffer(_SYSCubeID cid, struct _SYSVideoBuffer *vbuf)
{
    if (Runtime::checkUserPointer(vbuf, sizeof *vbuf) && CubeSlot::validID(cid))
	CubeSlot::instances[cid].setVideoBuffer(vbuf);
}

void _SYS_loadAssets(_SYSCubeID cid, struct _SYSAssetGroup *group)
{
    if (Runtime::checkUserPointer(group, sizeof *group)	&& CubeSlot::validID(cid))
	CubeSlot::instances[cid].loadAssets(group);
}

}  // extern "C"
