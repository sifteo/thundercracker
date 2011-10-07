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

int System::threadFn(void *param)
{
    /*
     * Maintain our virtual time clock, and update all parts of the
     * simulation. Also give the debugger a chance to run every so
     * often.
     */

    System *self = (System *) param;
    unsigned nCubes = self->opt_numCubes;
    bool debug = self->opt_cube0Debug && nCubes;

    self->time.init();
    if (debug)
        Cube::Debug::init(&self->cubes[0]);

    while (self->threadRunning) {
        unsigned batch = self->time.timestepTicks();

        while (batch--) {
            for (unsigned i = 0; i < nCubes; i++)
                self->cubes[i].tick();

            self->time.tick();

            if (debug)
                Cube::Debug::recordHistory();
        }

        if (debug)
            Cube::Debug::updateUI();
    }

    if (debug)
        Cube::Debug::exit();

    return 0;
}
