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
#include <SDL.h>
#include "macros.h"

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
        
    ALWAYS_INLINE void tick() {
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


class ElapsedTime {
 public:
    ElapsedTime(VirtualTime &vtime) : vtime(vtime) {}

    void capture() {
        currentRealMS = SDL_GetTicks();
    }

    void start() {
        virtualT = vtime.clocks;
        realMS = currentRealMS;
    }

    uint64_t virtualTicks() {
        return vtime.clocks - virtualT;
    }

    uint64_t realMsec() {
        return currentRealMS - realMS;
    }

    float virtualSeconds() {
        return virtualTicks() / (float)vtime.HZ;
    }

    float realSeconds() {
        return realMsec() * 1e-3;
    }

    float virtualRatio() {
        return realMsec() ? virtualSeconds() / realSeconds() : 0;
    }

 private:
    VirtualTime &vtime;
    uint64_t virtualT;
    uint32_t realMS;
    uint32_t currentRealMS;
};


class TimeGovernor {
 public:
    TimeGovernor(VirtualTime &vtime)
        : et(vtime) {}

    void start() {
        et.capture();
        et.start();
        secondsAhead = 0.0f;
    }

    void step() {
        /*
         * How fast are we running, on average? Keep an error value,
         * indicating how far ahead or behind real-time we're
         * running. If we need to slow down, we can insert a delay
         * here.
         *
         * If we're really far behind, give up on running full
         * speed. But if we're only a little bit behind, try to catch
         * up.
         */

        static const float maxSecondsBehind = 5.0f;

        et.capture();

        secondsAhead += et.virtualSeconds() - et.realSeconds();

        if (secondsAhead > 0)
            SDL_Delay(secondsAhead * 1000.0f);

        if (secondsAhead < -maxSecondsBehind)
            secondsAhead = -maxSecondsBehind;
       
        et.start();
    }

 private:
    ElapsedTime et;
    float secondsAhead;
};


#endif
