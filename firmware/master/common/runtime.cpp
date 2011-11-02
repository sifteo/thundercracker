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
uint32_t Event::accelChangeCubes;
uint32_t Event::touchCubes;


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

        case ASSET_DONE:
            while (assetDoneCubes) {
                uint32_t slot = Intrinsic::CLZ(assetDoneCubes);
                assetDone(slot);
                Atomic::And(assetDoneCubes, ~Intrinsic::LZ(slot));
            }
            break;

        case ACCEL_CHANGE:
            while (accelChangeCubes) {
                uint32_t slot = Intrinsic::CLZ(accelChangeCubes);
                accelChange(slot);
                Atomic::And(accelChangeCubes, ~Intrinsic::LZ(slot));
            }
            break;
            
        case TOUCH:
            while (touchCubes) {
                uint32_t slot = Intrinsic::CLZ(touchCubes);
                touch(slot);
                Atomic::And(touchCubes, ~Intrinsic::LZ(slot));
            }
            break;

        }
        Atomic::And(pending, ~Intrinsic::LZ(event));
    }

    dispatchInProgress = false;
}
