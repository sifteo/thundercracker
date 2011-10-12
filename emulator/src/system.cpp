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
                           i ? NULL : opt_cube0Flash))
            return false;

        /*
         * Link the neighbor sensors into a network
         */
        cubes[i].neighbors.attachCubes(cubes);

        /*
         * We cheat a little, for the sake of debugging ease, and give
         * each cube's radio a different hardwired default address. The
         * cube firmware is free to override this just as it normally
         * would, but this lets us get unique radio addresses for free
         * when running firmware that doesn't have a real address
         * assignment scheme implemented.
         */
        cubes[i].spi.radio.setAddressLSB(i);
    }

    if (opt_cube0Profile && opt_numCubes) {
        cubes[0].cpu.mProfileData = (Cube::CPU::profile_data *)
            calloc(CODE_SIZE, sizeof cubes[0].cpu.mProfileData[0]);
    }

    if (opt_cube0Trace && opt_numCubes) {
        cubes[0].cpu.traceFile = fopen(opt_cube0Trace, "w");
        if (!cubes[0].cpu.traceFile) {
            perror("Error opening trace file");
            return false;
        }
    }

    network.init();
    
    return true;
}

void System::start() {
    threadRunning = true;
    thread = SDL_CreateThread(threadFn, this);
}

void System::exit() {
    network.exit();

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
    ElapsedTime et(self->time);

    if (debug)
        Cube::Debug::init(&self->cubes[0]);

    et.capture();
    et.start();

    while (self->threadRunning) {
        if (debug) {
            /*
             * Debug loop. Handle breakpoints, exceptions, debug UI updates.
             * When debugging, the timestep may change dynamically.
             */

            bool tick0 = false;
            Cube::Hardware &debugCube = self->cubes[0];

            for (unsigned t = 0; t < self->time.timestepTicks(); t++) {
                tick0 |= debugCube.tick();
                for (unsigned i = 1; i < nCubes; i++)
                    self->cubes[i].tick();
                self->tick();
            }

            if (tick0 && Cube::Debug::updateUI()) {
                // Debugger requested that we quit
                self->threadRunning = false;
            }

        } else {
            // Faster loop for the non-debug case
                     
            unsigned batch = self->time.timestepTicks();
            while (batch--) {
                for (unsigned i = 0; i < nCubes; i++)
                    self->cubes[i].tick();
                self->tick();
            }

            /*
             * Do some periodic stats to stdout.
             *
             * XXX: The frontend should be doing this, not us. And it
             * should be part of the graphical output.
             */

            et.capture();
            if (et.realSeconds() > 1.0f) {
            
                printf("Simulator at %.2f %% actual speed -- [", et.virtualRatio() * 100.0f);
                for (unsigned i = 0; i < nCubes; i++)
                    printf("%8.2f", self->cubes[i].lcd.getWriteCount()  / et.virtualSeconds());
                printf(" ] FPS\n");

                et.start();
            }
        }
    }

    if (debug)
        Cube::Debug::exit();

    return 0;
}

void System::tick()
{
    // Everything but the cubes
    time.tick();
    network.tick(*this);
}
