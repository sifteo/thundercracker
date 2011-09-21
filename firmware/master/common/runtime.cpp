/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "runtime.h"
#include "cube.h"

using namespace Sifteo;

jmp_buf Runtime::jmpExit;

bool Event::dispatchInProgress;
uint32_t Event::pending;
uint32_t Event::assetDoneCubes;


void Runtime::run()
{
    if (setjmp(jmpExit))
	return;

    siftmain();
}

void Runtime::exit()
{
    longjmp(jmpExit, 1);
}

void Event::dispatch()
{
    /*
     * Skip event dispatch if we're already in an event handler
     */

    if (dispatchInProgress)
	return;
    dispatchInProgress = true;

    /*
     * Process events, by type
     */

    while (pending) {
	uint32_t event = Intrinsic::CLZ(pending);
	switch (event) {

	    /*
	     * Asset load completed.
	     *
	     * We track these by cube slot, since there are a limited
	     * number of cubes in the architecture and a less bounded
	     * number of asset groups. If a new asset group download
	     * begins before an event for the last one was sent, the
	     * event is suppressed.
	     */
	case ASSET_DONE:
	    while (assetDoneCubes) {
		uint32_t slot = Intrinsic::CLZ(assetDoneCubes);
		CubeSlot &cube = CubeSlot::instances[slot];
		_SYSAssetGroup *group = cube.getLastAssetGroup();

		if (group && cube.isAssetGroupLoaded(group))
		    assetDone(group);

		Atomic::And(assetDoneCubes, ~Intrinsic::LZ(slot));
	    }
	    break;

	}
	Atomic::And(pending, ~Intrinsic::LZ(event));
    }

    dispatchInProgress = false;
}
