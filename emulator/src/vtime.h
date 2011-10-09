/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _VTIME_H
#define _VTIME_H

#include <stdint.h>
#include <string.h>

class VirtualTime {
 public:
    // Master clock rate. Must be equal to the cube's system clock.
    static const unsigned HZ = 16000000;

    // Timestep size for simulation, in real milliseconds
    static const unsigned TIMESTEP = 10;

    uint64_t clocks;

    static unsigned usec(uint64_t u) {       
        return HZ * u / 1000000ULL;
    }

    static unsigned nsec(uint64_t n) {       
        return HZ * n / 1000000000ULL;
    }

    static unsigned hz(unsigned h) {    
        return HZ / h;
    }

    static float toSeconds(uint64_t cycles) {
        return cycles / (float)HZ;
    }

    static float clockMHZ() {
        return HZ / 1e6;
    }
    
    void init() {
        clocks = 0;
        run();
    }
        
    void tick() {
        clocks++;
    }

    float elapsedSeconds() {
        return toSeconds(clocks);
    }

    void setTargetRate(unsigned realTimeHz) {
        targetRate = realTimeHz;
    }

    void run() {
        setTargetRate(HZ);
    }

    void stop() {
        setTargetRate(0);
    }

    bool isPaused() {
        return targetRate == 0;
    }

    unsigned timestepTicks() {
        unsigned t = TIMESTEP * targetRate / 1000;
        return t ? t : 1;
    }

private:
    unsigned targetRate;
};

#endif
