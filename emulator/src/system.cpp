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
        opt_continueOnException(false),
        opt_turbo(false),
        opt_cube0Debug(NULL),
        threadRunning(false),
        mIsInitialized(false),
        mIsStarted(false)
        {}


bool System::init()
{
    if (mIsInitialized)
        return true;
        
    if (opt_cubeFirmware.empty() && (!opt_cube0Profile.empty() || 
                                     opt_cube0Debug)) {
        /*
         * We want to disable our debugging features when using the
         * built-in binary translated firmware. The debugger won't really
         * work properly in SBT mode anyway, but we additionally want to
         * disable it in order to make it harder to reverse engineer our
         * firmware. Of course, any dedicated reverse engineer could just
         * disable this test easily :)
         */
        fprintf(stderr, "Debug features only available if a firmware image is provided.\n");
        return false;
    }

    /*
     * To save on clutter in the trace file, we intentionally only set up tracing
     * for the initial number of cubes, not the maximum number of cubes. Additional cubes
     * added dynamically will not be traced.
     */
    for (unsigned i = 0; i < opt_numCubes; i++) {
        char cubeName[32];

        snprintf(cubeName, sizeof cubeName, "cube_%02d", i);
        tracer.vcd.enterScope(cubeName);

        snprintf(cubeName, sizeof cubeName, "c%02d_", i);
        tracer.vcd.setNamePrefix(cubeName);

        cubes[i].initVCD(tracer.vcd);
        tracer.vcd.leaveScope();
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
                        (id != 0 || opt_cube0Flash.empty()) ? NULL : opt_cube0Flash.c_str()))
        return false;

    cubes[id].cpu.id = id;
    
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
        
    network.exit();

    if (mIsStarted) {
        stopThread();
        mIsStarted = false;

        if (opt_cube0Debug)
            Cube::Debug::exit();
    }
    
    for (unsigned i = 0; i < opt_numCubes; i++)
        exitCube(i);

    if (!opt_cube0Profile.empty())
        Cube::Debug::writeProfile(&cubes[0].cpu, opt_cube0Profile.c_str());

    tracer.close();
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

    // Seed PRNG per-thread
    srand(glfwGetTime() * 1e6);
        
    while (self->threadRunning) {
        
        /*
         * Pick one of several specific tick batch loops. This keeps the loop tight by
         * eliminating unused features when possible.
         *
         * All batch loops are marked NEVER_INLINE, so that they will show up separately
         * in profilers.
         */
         
        if (debug) {
            self->tickLoopDebug();
        } else if (nCubes < 1 || !self->cubes[0].cpu.sbt || self->cubes[0].cpu.mProfileData || Tracer::isEnabled()) {
            self->tickLoopGeneral();
        } else {
            self->tickLoopFastSBT();
        }

        /*
         * Use TimeGovernor to keep us running no faster than real-time.
         * It keeps a running total of how far ahead or behind we are,
         * so that on average we can track real-time accurately assuming
         * we have the CPU power to do so.
         */

        if (!self->opt_turbo)
            gov.step();
    }
}

ALWAYS_INLINE void System::tick(unsigned count)
{
    time.tick(count);
    
    /*
     * Note: It's redundant to be passing our 'time' pointer to network.tick
     * separately, but there are two reasons this is really important:
     *
     *   1. The process of dereferencing VirtualTime from a TickDeadline was
     *      causing a very slow memory access every tick, at least on my system.
     *      Eliminating this memory access increased my sim performance by ~10%!
     *
     *   2. SystemNetwork can't access System::time directly, even though it's
     *      public, because it would introduce a circular dependency.
     *
     * So, we pass 'time' separately to this (inlined) function.
     */
    network.tick(*this, &time);
}

NEVER_INLINE void System::tickLoopDebug()
{
    /*
     * Debug loop. Handle breakpoints, exceptions, debug UI updates.
     * When debugging, the timestep may change dynamically.
     */

    bool tick0 = false;
    Cube::Hardware &debugCube = cubes[0];
    unsigned nCubes = opt_numCubes;

    for (unsigned t = 0; t < time.timestepTicks(); t++) {
        bool debugCPUTicked = false;
        debugCube.tick(&debugCPUTicked);
        
        if (debugCPUTicked) {
            tick0 = true;
            Cube::Debug::recordHistory();
        }

        for (unsigned i = 1; i < nCubes; i++)
            cubes[i].tick();
        tick();
        tracer.tick(time);
    }

    if (tick0 && Cube::Debug::updateUI()) {
        // Debugger requested that we quit
        threadRunning = false;
    }
}

NEVER_INLINE void System::tickLoopGeneral()
{
    /*
     * Faster loop for the non-debug case, but when we still might be using interpreted firmware,
     * profiling, or tracing.
     */
            
    unsigned batch = time.timestepTicks();
    unsigned nCubes = opt_numCubes;
    
    while (batch--) {
        for (unsigned i = 0; i < nCubes; i++)
            cubes[i].tick();
        tick();
        tracer.tick(time);
    }
}

NEVER_INLINE void System::tickLoopFastSBT()
{
    /*
     * Fastest path: No debugging, no tracing, SBT only,
     * and advance by more than one tick when we can.
     */
            
    unsigned batch = time.timestepTicks();
    unsigned nCubes = opt_numCubes;
    unsigned stepSize = 1;
    
    while (batch) {
        unsigned nextStep;

        batch -= stepSize;
        nextStep = batch;
        
#if 0
        // Debugging batch sizes
        printf("batch %d, cubes: %d @%04x, %d @%04x, %d @%04x\n",
                stepSize,
                cubes[0].cpu.mTickDelay, cubes[0].cpu.mPC,
                cubes[1].cpu.mTickDelay, cubes[1].cpu.mPC,
                cubes[2].cpu.mTickDelay, cubes[2].cpu.mPC);        
#endif
        
        for (unsigned i = 0; i < nCubes; i++)
            nextStep = std::min(nextStep, cubes[i].tickFastSBT(stepSize));
        
        tick(stepSize);
        stepSize = std::min((uint64_t)nextStep, network.deadlineRemaining());
    }
}

