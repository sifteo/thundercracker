/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include "system.h"
#include "ostime.h"
#include "system_cubes.h"


bool SystemCubes::init(System *sys)
{
    this->sys = sys;
    deadlineSync.init(&sys->time, &mThreadRunning);

    if (sys->opt_cubeFirmware.empty() && (!sys->opt_cube0Profile.empty() || 
                                           sys->opt_cube0Debug)) {
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
    for (unsigned i = 0; i < sys->opt_numCubes; i++) {
        char cubeName[32];

        snprintf(cubeName, sizeof cubeName, "cube_%02d", i);
        sys->tracer.vcd.enterScope(cubeName);

        snprintf(cubeName, sizeof cubeName, "c%02d_", i);
        sys->tracer.vcd.setNamePrefix(cubeName);

        sys->cubes[i].initVCD(sys->tracer.vcd);
        sys->tracer.vcd.leaveScope();
    }

    // Initialize default cubes, and wake them from sleep
    for (unsigned i = 0; i < sys->opt_numCubes; i++)
        if (!initCube(i, true))
            return false;
    
    return true;
}

void SystemCubes::setNumCubes(unsigned n)
{
    if (n == sys->opt_numCubes)
        return;

    // Must change opt_numCubes only while our thread is stopped!
    tthread::lock_guard<tthread::mutex> guard(mBigCubeLock);

    // Initialize any new cubes
    while (sys->opt_numCubes < n)
        if (initCube(sys->opt_numCubes))
            sys->opt_numCubes++;
        else
            break;

    // Nothing special needed to remove a cube
    ASSERT(sys->opt_numCubes >= n);
    sys->opt_numCubes = n;
}

void SystemCubes::resetCube(unsigned id)
{
    tthread::lock_guard<tthread::mutex> guard(mBigCubeLock);
    sys->cubes[id].reset();
}

bool SystemCubes::initCube(unsigned id, bool wakeFromSleep)
{
    const char *firmware = sys->opt_cubeFirmware.empty()
        ? NULL : sys->opt_cubeFirmware.c_str();

    ASSERT(sys->flash.data);
    if (!sys->cubes[id].init(&sys->time, firmware,
        &sys->flash.data->cubes[id], wakeFromSleep))
        return false;

    sys->cubes[id].cpu.id = id;
    
    if (id == 0 && !sys->opt_cube0Profile.empty()) {
        Cube::CPU::profile_data *pd;
        size_t s = CODE_SIZE * sizeof pd[0];
        pd = (Cube::CPU::profile_data *) malloc(s);
        memset(pd, 0, s);
        sys->cubes[0].cpu.mProfileData = pd;
    }

    sys->cubes[id].neighbors.attachCubes(sys->cubes);

    /*
     * We cheat a little, for the sake of debugging ease, and give
     * each cube's radio a different hardwired default address. The
     * cube firmware is free to override this just as it normally
     * would, but this lets us get unique radio addresses for free
     * when running firmware that doesn't have a real address
     * assignment scheme implemented.
     */
    sys->cubes[id].spi.radio.setAddressLSB(id);

    return true;
}

void SystemCubes::start()
{
    if (sys->opt_cube0Debug) {
        Cube::Debug::init();
        Cube::Debug::stopOnException = !sys->opt_continueOnException;
    }

    mThreadRunning = true;
    __asm__ __volatile__ ("" : : : "memory");
    mThread = new tthread::thread(threadFn, this);
}

void SystemCubes::stop()
{
    mThreadRunning = false;
    __asm__ __volatile__ ("" : : : "memory");
    deadlineSync.wake();
    mThread->join();
    delete mThread;
    mThread = 0;

    if (sys->opt_cube0Debug)
        Cube::Debug::exit();
}

void SystemCubes::exit()
{
    if (!sys->opt_cube0Profile.empty())
        Cube::Debug::writeProfile(&sys->cubes[0].cpu, sys->opt_cube0Profile.c_str());
}

void SystemCubes::threadFn(void *param)
{
    /*
     * Maintain our virtual time clock, and update all parts of the
     * simulation. Also give the debugger a chance to run every so
     * often.
     *
     * The number of cubes can't change while this thread is executing.
     */

    SystemCubes *self = (SystemCubes *) param;
    System *sys = self->sys;
    unsigned nCubes = sys->opt_numCubes;
    bool debug = sys->opt_cube0Debug && nCubes;

    TimeGovernor gov;
    gov.start(&sys->time);
    
    if (debug)
        Cube::Debug::attach(&sys->cubes[0]);

    // Seed PRNG per-thread
    srand(OSTime::clock() * 1e6);

    while (self->mThreadRunning) {
        /*
         * Pick one of several specific tick batch loops. This keeps the loop tight by
         * eliminating unused features when possible.
         *
         * All batch loops are marked NEVER_INLINE, so that they will show up separately
         * in profilers.
         */

        self->mBigCubeLock.lock();
        if (nCubes == 0) {
            self->tickLoopEmpty();
        } else if (debug) {
            self->tickLoopDebug();
        } else if (!sys->cubes[0].cpu.sbt || sys->cubes[0].cpu.mProfileData || Tracer::isEnabled()) {
            self->tickLoopGeneral();
        } else {
            self->tickLoopFastSBT();
        }
        self->mBigCubeLock.unlock();

        /*
         * Use TimeGovernor to keep us running no faster than real-time.
         * It keeps a running total of how far ahead or behind we are,
         * so that on average we can track real-time accurately assuming
         * we have the CPU power to do so.
         */

        if (!sys->opt_turbo)
            gov.step();
    }
}

ALWAYS_INLINE void SystemCubes::tick(unsigned count)
{
    sys->time.tick(count);
    deadlineSync.tick();
}

NEVER_INLINE void SystemCubes::tickLoopDebug()
{
    /*
     * Debug loop. Handle breakpoints, exceptions, debug UI updates.
     * When debugging, the timestep may change dynamically.
     */

    bool tick0 = false;
    System *sys = this->sys;
    Cube::Hardware &debugCube = sys->cubes[0];
    unsigned nCubes = sys->opt_numCubes;

    for (unsigned t = 0; t < sys->time.timestepTicks(); t++) {
        bool debugCPUTicked = false;
        debugCube.tick(&debugCPUTicked);
        
        if (debugCPUTicked) {
            tick0 = true;
            Cube::Debug::recordHistory();
        }

        for (unsigned i = 1; i < nCubes; i++)
            sys->cubes[i].tick();
        tick();
        sys->tracer.tick(sys->time);
    }

    if (tick0 && Cube::Debug::updateUI()) {
        // Debugger requested that we quit
        mThreadRunning = false;
    }
}

NEVER_INLINE void SystemCubes::tickLoopGeneral()
{
    /*
     * Faster loop for the non-debug case, but when we still might be using interpreted firmware,
     * profiling, or tracing.
     */

    System *sys = this->sys;
    unsigned batch = sys->time.timestepTicks();
    unsigned nCubes = sys->opt_numCubes;
    
    while (batch--) {
        for (unsigned i = 0; i < nCubes; i++)
            sys->cubes[i].tick();
        tick();
        sys->tracer.tick(sys->time);
    }
}

NEVER_INLINE void SystemCubes::tickLoopFastSBT()
{
    /*
     * Fastest path: No debugging, no tracing, SBT only,
     * and advance by more than one tick when we can.
     */

    System *sys = this->sys;
    unsigned batch = sys->time.timestepTicks();
    unsigned nCubes = sys->opt_numCubes;
    unsigned stepSize = 1;

    /*
     * Run until our batch is empty, or someone tells us to stop.
     *
     * Note: stepSize is only equal to 0 in exceptional cases, such as
     *       if our thread is exiting and deadlineSync is halted on the same
     *       clock tick, preventing us from making forward progress.
     */

    while (batch && stepSize) {
        unsigned nextStep;

        batch -= stepSize;
        nextStep = batch;

        for (unsigned i = 0; i < nCubes; i++) {
            Cube::Hardware &cube = sys->cubes[i];
            if (!cube.isSleeping())
                nextStep = std::min(nextStep, sys->cubes[i].tickFastSBT(stepSize));
        }

        tick(stepSize);
        stepSize = std::min(nextStep, (unsigned)deadlineSync.remaining());
    }
}

NEVER_INLINE void SystemCubes::tickLoopEmpty()
{
    /*
     * Special-case tick loop for when the cube emulator is emulating no cubes.
     * We still need to advance the clock, and operate deadlineSync.
     */

    System *sys = this->sys;
    unsigned batch = sys->time.timestepTicks();
    unsigned stepSize = 1;

    while (batch && stepSize) {
        unsigned nextStep;
        batch -= stepSize;
        nextStep = batch;
        tick(stepSize);
        stepSize = std::min(nextStep, (unsigned)deadlineSync.remaining());
    }
}
