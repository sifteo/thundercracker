/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * The whole system- all the hardware we're simulating.
 * This module brings it all together.
 */

#ifndef _SYSTEM_H
#define _SYSTEM_H

#include <string>
#include <glfw.h>
#include "vtime.h"
#include "cube_hardware.h"
#include "system_network.h"


class System {
 public:
    VirtualTime time;

    static const unsigned DEFAULT_CUBES = 2;
    static const unsigned MAX_CUBES = 32;

    Cube::Hardware cubes[MAX_CUBES];

    // Options; can be set prior to init
    unsigned opt_numCubes;
    std::string opt_cubeFirmware;
    bool opt_noThrottle;

    // Global debug options
    std::string opt_cubeTrace;
    bool opt_continueOnException;

    // Debug options, applicable to cube 0 only
    bool opt_cube0Debug;
    std::string opt_cube0Flash;
    std::string opt_cube0Profile;

    System();
    
    bool init();
    void start();
    void exit();
    void setNumCubes(unsigned n);
    void setTraceMode(bool t);
    
    bool isRunning() {
        return threadRunning;
    }
    
    bool isTracing() {
        return mIsTracing;
    }

    // Use with care... They must remain exactly paired.
    void startThread();
    void stopThread();

 private: 
    static void threadFn(void *param);
    bool initCube(unsigned id);
    void exitCube(unsigned id);
    void tick();

    GLFWthread thread;
    bool threadRunning;

    FILE *traceFile;
    bool mIsTracing;
    bool mIsInitialized;
    bool mIsStarted;
    
    SystemNetwork network;
};

#endif
