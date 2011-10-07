/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "system.h"
#include "cube_debug.h"


bool System::init() {
    for (unsigned i = 0; i < opt_numCubes; i++) {
        if (!cubes[i].init(&time,
                           opt_cubeFirmware,
                           i ? NULL : opt_cube0Flash,
                           opt_netHost, opt_netPort))
            return false;
    }

    if (opt_cube0Trace) {
        cubes[0].cpu.traceFile = fopen(opt_cube0Trace, "w");
        if (!cubes[0].cpu.traceFile) {
            perror("Error opening trace file");
            return false;
        }
    }
    
    return true;
}

void System::start() {
    threadRunning = true;
    thread = SDL_CreateThread(threadFn, this);
}

void System::exit() {
    threadRunning = false;
    SDL_WaitThread(thread, NULL);

    for (unsigned i = 0; i < opt_numCubes; i++)
        cubes[i].exit();

    if (opt_cube0Profile)
        Cube::Debug::writeProfile(&cubes[0].cpu, opt_cube0Profile);

    if (opt_cube0Trace)
        fclose(cubes[0].cpu.traceFile);
}

void System::tick() {
    for (unsigned i = 0; i < opt_numCubes; i++)
        cubes[i].tick();

    if (opt_cube0Debug) {
        Cube::Debug::recordHistory();
        Cube::Debug::updateUI();
    }   
}

int System::threadFn(void *param)
{
    return 0;
}
