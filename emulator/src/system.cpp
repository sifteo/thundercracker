/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <string.h>
#include <stdlib.h>
#include "system.h"
#include "cube_debug.h"

System::System()
        : opt_numCubes(DEFAULT_CUBES),
        opt_noThrottle(false),
        opt_continueOnException(false),
        opt_cube0Debug(NULL),
        threadRunning(false),
        traceFile(NULL),
        mIsTracing(false),
        mIsInitialized(false),
        mIsStarted(false)
        {}


bool System::init()
{
    if (mIsInitialized)
        return true;
        
    /*
     * We want to disable our debugging features when using the
     * built-in binary translated firmware. The debugger won't really
     * work properly in SBT mode anyway, but we additionally want to
     * disable it in order to make it harder to reverse engineer our
     * firmware. Of course, any dedicated reverse engineer could just
     * disable this test easily :)
     */

    if (opt_cubeFirmware.empty() && (!opt_cube0Profile.empty() || 
                                     !opt_cubeTrace.empty() ||
                                     opt_cube0Debug)) {
        fprintf(stderr, "Debug features only available if a firmware image is provided.");
        return false;
    }

    if (!opt_cubeTrace.empty()) {
        traceFile = fopen(opt_cubeTrace.c_str(), "w");
        if (!traceFile) {
            perror("Error opening trace file");
            return false;
        }
    } else {
        traceFile = NULL;
    }

    for (unsigned i = 0; i < opt_numCubes; i++)
        if (!initCube(i))
            return false;
    
    time.init();
    network.init(&time);

    mIsInitialized = true;
    return true;
}

void System::startThread()
{
    threadRunning = true;
    __asm__ __volatile__ ("" : : : "memory");
    thread = glfwCreateThread(threadFn, this);
    __asm__ __volatile__ ("" : : : "memory");
}

void System::stopThread()
{
    threadRunning = false;
    __asm__ __volatile__ ("" : : : "memory");
    glfwWaitThread(thread, GLFW_WAIT);
    __asm__ __volatile__ ("" : : : "memory");
}

void System::setNumCubes(unsigned n)
{
    if (n == opt_numCubes)
        return;

    // Must change opt_numCubes only while our thread is stopped!
    stopThread();
    
    while (opt_numCubes > n)
        exitCube(--opt_numCubes);

    while (opt_numCubes < n)
        if (initCube(opt_numCubes))
            opt_numCubes++;
        else
            break;

    startThread();
}

bool System::initCube(unsigned id)
{
    if (!cubes[id].init(&time, opt_cubeFirmware.empty() ? NULL : opt_cubeFirmware.c_str(),
                        !id || opt_cube0Flash.empty() ? NULL : opt_cube0Flash.c_str()))
        return false;

    cubes[id].cpu.id = id;
    cubes[id].cpu.traceFile = traceFile;
    cubes[id].cpu.isTracing = mIsTracing;
    
    if (id == 0 && !opt_cube0Profile.empty()) {
        Cube::CPU::profile_data *pd;
        size_t s = CODE_SIZE * sizeof pd[0];
        pd = (Cube::CPU::profile_data *) malloc(s);
        memset(pd, 0, s);
        cubes[0].cpu.mProfileData = pd;
    }

    cubes[id].neighbors.attachCubes(cubes);

    /*
     * We cheat a little, for the sake of debugging ease, and give
     * each cube's radio a different hardwired default address. The
     * cube firmware is free to override this just as it normally
     * would, but this lets us get unique radio addresses for free
     * when running firmware that doesn't have a real address
     * assignment scheme implemented.
     */
    cubes[id].spi.radio.setAddressLSB(id);        

    return true;
}

void System::exitCube(unsigned id)
{
    cubes[id].exit();
}

void System::start()
{
    if (!mIsInitialized || mIsStarted)
        return;
    mIsStarted = true;
        
    if (opt_cube0Debug) {
        Cube::Debug::init();
        Cube::Debug::stopOnException = !opt_continueOnException;
    }

    startThread();
}

void System::exit()
{
    if (!mIsInitialized)
        return;
    mIsInitialized = false;
    mIsStarted = false;
        
    network.exit();

    if (mIsStarted) {
        stopThread();

        if (opt_cube0Debug)
            Cube::Debug::exit();
    }
    
    for (unsigned i = 0; i < opt_numCubes; i++)
        exitCube(i);

    if (!opt_cube0Profile.empty())
        Cube::Debug::writeProfile(&cubes[0].cpu, opt_cube0Profile.c_str());

    if (traceFile) {
        fclose(traceFile);
        traceFile = NULL;
    }
}

void System::threadFn(void *param)
{
    /*
     * Maintain our virtual time clock, and update all parts of the
     * simulation. Also give the debugger a chance to run every so
     * often.
     *
     * The number of cubes can't change while this thread is executing.
     */

    System *self = (System *) param;
    unsigned nCubes = self->opt_numCubes;
    bool debug = self->opt_cube0Debug && nCubes;

    TimeGovernor gov;
    gov.start(&self->time);
    
    if (debug)
        Cube::Debug::attach(&self->cubes[0]);

    while (self->threadRunning) {
        if (debug) {
            /*
             * Debug loop. Handle breakpoints, exceptions, debug UI updates.
             * When debugging, the timestep may change dynamically.
             */

            bool tick0 = false;
            Cube::Hardware &debugCube = self->cubes[0];

            for (unsigned t = 0; t < self->time.timestepTicks(); t++) {
                if (debugCube.tick()) {
                    tick0 = true;
                    Cube::Debug::recordHistory();
                }
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
        }

        /*
         * Use TimeGovernor to keep us running no faster than real-time.
         * It keeps a running total of how far ahead or behind we are,
         * so that on average we can track real-time accurately assuming
         * we have the CPU power to do so.
         */

        if (!self->opt_noThrottle)
            gov.step();
    }
}

ALWAYS_INLINE void System::tick()
{
    // Everything but the cubes
    time.tick();
    network.tick(*this);
}

void System::setTraceMode(bool t)
{
    mIsTracing = !opt_cubeTrace.empty() && traceFile && t;

    if (opt_numCubes) {
        bool startStop = mIsStarted;
    
        if (startStop)
            stopThread();
        for (unsigned i = 0; i < opt_numCubes; i++)
            cubes[i].cpu.isTracing = mIsTracing;
        if (startStop)
            startThread();
    }
}
