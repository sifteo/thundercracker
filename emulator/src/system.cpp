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
#include "mc_gdbserver.h"


System::System()
        : opt_headless(false),
        opt_numCubes(DEFAULT_CUBES),
        opt_whiteBackground(false),
        opt_windowWidth(800),
        opt_windowHeight(600),
        opt_continueOnException(false),
        opt_turbo(false),
        opt_lockRotationByDefault(false),
        opt_noCubeReconnect(false),
        opt_paintTrace(false),
        opt_svmTrace(false),
        opt_svmFlashStats(false),
        opt_gdbServerPort(0),
        opt_cube0Debug(NULL),
        opt_mute(false),
        mIsInitialized(false),
        mIsStarted(false)
        {}


bool System::init()
{
    if (mIsInitialized)
        return true;

    if (!flash.init(opt_flashFilename.empty() ? NULL : opt_flashFilename.c_str()))
        return false;

    if (!sc.init(this))
        return false;

    if (!smc.init(this))
        return false;

    time.init();

    mIsInitialized = true;
    return true;
}

void System::setNumCubes(unsigned n)
{
    sc.setNumCubes(n);
}

void System::resetCube(unsigned id)
{
    sc.resetCube(id);
}

void System::fullResetCube(unsigned id)
{
    sc.fullResetCube(id);
}

bool System::isTraceAllowed()
{   
    /*
     * Tracing allowed only in non-SBT mode normally
     * you can only trace if you're a developer who built
     * the firmware yourself. Normally it's bad to trace in SBT mode,
     * both because the traces will be less useful, and because it could
     * make it easier to reverse engineer our translated firmware.
     *
     * This test can be overridden (e.g. to debug the SBT code itself)
     * with a #define at compile-time.
     */
#ifdef ALWAYS_ALLOW_TRACE
    return true;
#else
    return !opt_cubeFirmware.empty();
#endif
}

void System::start()
{
    if (!mIsInitialized || mIsStarted)
        return;
    mIsStarted = true;

    tracer.setEnabled(opt_traceEnabledAtStartup && isTraceAllowed());

    sc.start();
    smc.start();

    if (opt_gdbServerPort)
        GDBServer::start(opt_gdbServerPort);
}

void System::exit()
{
    if (!mIsInitialized)
        return;
    mIsInitialized = false;

    if (mIsStarted) {
        if (opt_gdbServerPort)
            GDBServer::stop();

        smc.stop();
        sc.stop();

        mIsStarted = false;
    }

    smc.exit();
    sc.exit();
    flash.exit();
    tracer.close();
}
