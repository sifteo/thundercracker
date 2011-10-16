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

#include <glfw.h>
#include "vtime.h"
#include "cube_hardware.h"
#include "system_network.h"


class System {
 public:
    VirtualTime time;

    static const unsigned DEFAULT_CUBES = 1;
    static const unsigned MAX_CUBES = 32;

    Cube::Hardware cubes[MAX_CUBES];

    // Options; can be set prior to init
    unsigned opt_numCubes;
    const char *opt_cubeFirmware;
    bool opt_noThrottle;

    // Debug options, applicable to cube 0 only
    bool opt_cube0Debug;
    const char *opt_cube0Flash;
    const char *opt_cube0Trace;
    const char *opt_cube0Profile;

    System()
        : opt_numCubes(DEFAULT_CUBES), opt_cubeFirmware(NULL),
        opt_noThrottle(false), opt_cube0Debug(false),
        opt_cube0Flash(NULL), opt_cube0Trace(NULL),
        opt_cube0Profile(NULL), threadRunning(false)
        {}

    bool init();
    void start();
    void exit();

    bool isRunning() {
        return threadRunning;
    }

 private:
    static void threadFn(void *param);
    void tick();

    GLFWthread thread;
    bool threadRunning;

    SystemNetwork network;
};

#endif
