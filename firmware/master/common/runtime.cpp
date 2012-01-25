/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "runtime.h"
#include "cube.h"
#include "neighbors.h"

using namespace Sifteo;

jmp_buf Runtime::jmpExit;

bool Event::dispatchInProgress;
uint32_t Event::pending;
uint32_t Event::eventCubes[EventBits::COUNT];
bool Event::paused = false;


void Runtime::run()
{
    if (setjmp(jmpExit))
        return;

#ifndef BUILD_UNIT_TEST
    siftmain();
#endif
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

    if (dispatchInProgress || paused)
        return;
    dispatchInProgress = true;

    /*
     * Process events, by type
     */

    while (pending) {
        EventBits::ID event = (EventBits::ID)Intrinsic::CLZ(pending);

		while (eventCubes[event]) {
                _SYSCubeID slot = Intrinsic::CLZ(eventCubes[event]);
                if (event <= EventBits::LAST_CUBE_EVENT) {
                    callCubeEvent(event, slot);
                } else if (event == EventBits::NEIGHBOR) {
                    NeighborSlot::instances[slot].computeEvents();
                }
                
                Atomic::And(eventCubes[event], ~Intrinsic::LZ(slot));
            }
        Atomic::And(pending, ~Intrinsic::LZ(event));
    }

    dispatchInProgress = false;
}
