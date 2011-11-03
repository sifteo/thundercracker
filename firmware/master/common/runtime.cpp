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
uint32_t Event::eventCubes[_SYS_EVENT_CNT];


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
        _SYS_EventType event = (_SYS_EventType)Intrinsic::CLZ(pending);

		while (eventCubes[event]) {
                uint32_t slot = Intrinsic::CLZ(eventCubes[event]);
                callEvent(event, slot);
                Atomic::And(eventCubes[event], ~Intrinsic::LZ(slot));
            }
        Atomic::And(pending, ~Intrinsic::LZ(event));
    }

    dispatchInProgress = false;
}
