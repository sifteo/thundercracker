/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * The whole system- all the hardware we're simulating.
 * This module brings it all together.
 */

#ifndef _SYSTEM_H
#define _SYSTEM_H

#include <string>
#include <sifteo/abi.h>

#include "vtime.h"
#include "cube_hardware.h"
#include "system_cubes.h"
#include "system_mc.h"
#include "tracer.h"
#include "tinythread.h"
#include "flash_storage.h"


class System {
 public:

    VirtualTime time;

    static const unsigned DEFAULT_CUBES = 3;
    static const unsigned MAX_CUBES = _SYS_NUM_CUBE_SLOTS;

    Cube::Hardware cubes[MAX_CUBES];
    Tracer tracer;
    FlashStorage flash;

    // Static Options; can be set prior to init only
    bool opt_headless;
    unsigned opt_numCubes;
    std::string opt_cubeFirmware;
    std::string opt_flashFilename;
    std::string opt_launcherFilename;
    std::string opt_waveoutFilename;

    // UI options
    bool opt_whiteBackground;
    int opt_windowWidth;
    int opt_windowHeight;

    // Global debug options
    bool opt_continueOnException;
    bool opt_turbo;
    bool opt_lockRotationByDefault;
    bool opt_radioTrace;
    bool opt_traceEnabledAtStartup;
    bool opt_noCubeReconnect;
    bool opt_flushLogs;

    // Master firmware debug options
    bool opt_paintTrace;

    // SVM options
    bool opt_svmTrace;
    bool opt_svmFlashStats;
    bool opt_svmStackMonitor;
    unsigned opt_gdbServerPort;

    // Debug options, applicable to cube 0 only
    bool opt_cube0Debug;
    std::string opt_cube0Profile;

    // Other options
    bool opt_mute;
    double opt_radioNoise;

    bool init();
    void start();
    void exit();
    void setNumCubes(unsigned n);
    void resetCube(unsigned id);
    void fullResetCube(unsigned id);

    bool isRunning() {
        return mIsStarted;
    }

    bool isTraceAllowed();

    DeadlineSynchronizer &getCubeSync() {
        return sc.deadlineSync;
    }

    void stopCubesOnly() {
        sc.stop();
    }

 private:
    System();
    
    bool mIsInitialized;
    bool mIsStarted;

    SystemCubes sc;
    SystemMC smc;

public:

    inline static System& getInstance() {
        static System sys;
        return sys;
    }


};

#endif
