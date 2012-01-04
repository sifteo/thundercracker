/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _VTIME_H
#define _VTIME_H

#include "macros.h"

#include <stdint.h>
#include <string.h>
#include <glfw.h>


class VirtualTime {
    /*
     * Tracks a virtual clock, measured in clock ticks.
     */

 public:
    // Master clock rate. Must be equal to the cube's system clock.
    static const uint64_t HZ = 16000000;

    // Timestep size for simulation, in real milliseconds
    static const unsigned TIMESTEP = 10;

    uint64_t clocks;

    static uint64_t msec(uint64_t u) {       
        return HZ * u / 1000ULL;
    }

    static uint64_t usec(uint64_t u) {       
        return HZ * u / 1000000ULL;
    }

    static uint64_t nsec(uint64_t n) {       
        return HZ * n / 1000000000ULL;
    }

    static uint64_t hz(unsigned h) {    
        return HZ / h;
    }

    static double toSeconds(uint64_t cycles) {
        return cycles / (double)HZ;
    }

    static double clockMHZ() {
        return HZ / 1e6;
    }
    
    void init() {
        clocks = 0;
        run();
    }
        
    ALWAYS_INLINE void tick() {
        clocks++;
    }

    double elapsedSeconds() const {
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

    bool isPaused() const {
        return targetRate == 0;
    }

    unsigned timestepTicks() const {
        unsigned t = TIMESTEP * targetRate / 1000;
        return t ? t : 1;
    }

private:
    unsigned targetRate;
};


class ElapsedTime {
    /*
     * A timer which measures elapsed virtual and real time, from a
     * common reference point.
     */

 public:
    void init(const VirtualTime *_vtime) {
        vtime = _vtime;
        capture();
    }
    
    void capture() {
        currentRealS = glfwGetTime();
    }

    void capture(const ElapsedTime &source) {
        /*
         * Allows optimization to reduce the number of system calls
         * when many timers are captured at the same time.
         */
        currentRealS = source.currentRealS;
    }

    void start() {
        virtualT = vtime->clocks;
        realS = currentRealS;
    }

    uint64_t virtualTicks() {
        return vtime->clocks - virtualT;
    }

    uint64_t realMsec() {
        return realSeconds() * 1e3;
    }

    double virtualSeconds() {
        return virtualTicks() / (double)vtime->HZ;
    }

    double realSeconds() {
        return currentRealS - realS;
    }

    double virtualRatio() {
        double rs = realSeconds();
        return rs ? virtualSeconds() / rs : 0;
    }

 private:
    const VirtualTime *vtime;
    uint64_t virtualT;
    double realS;
    double currentRealS;
};


class TimeGovernor {
    /*
     * The time governor slows us down if we're running faster than
     * real-time, by keeping a long-term average of how many seconds
     * ahead or behind we are.
     */

 public:
    void start(const VirtualTime *vtime) {
        et.init(vtime);
        et.start();
        secondsAhead = 0.0;
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

        static const double maxDeviation = 0.75;
        static const double minSleep = 0.01;

        et.capture();

        secondsAhead += et.virtualSeconds() - et.realSeconds();

        /*
         * Fully reset if we're lagging/leading too much, rather than clamping to our limit.
         * (Among other things, this avoids catch-up delay when coming out of Turbo)
         */        

        if (secondsAhead < -maxDeviation || secondsAhead > maxDeviation)
            secondsAhead = 0;

        if (secondsAhead > minSleep)
            glfwSleep(secondsAhead);
       
        et.start();
    }

 private:
    ElapsedTime et;
    double secondsAhead;
};


class TickDeadline {
    /*
     * For efficient hardware simulation, we want to run as little
     * code as possible per-tick. Deadlines are used to schedule
     * future events. If a peripheral knows it needs to be called no
     * later than a particular clock tick, it can move the deadline
     * forward.
     */

 public:
    void init(const VirtualTime *_vtime) {
        vtime = _vtime;
        reset();
    }

    void reset() {
        ticks = 0xFFFFFFFFFFFFFFFFULL;
    }

    void set(uint64_t latest) {
        ticks = MIN(latest, ticks);
    }

    uint64_t setRelative(uint64_t diff) {
        uint64_t absolute = vtime->clocks + diff;
        set(absolute);
        return absolute;
    }

    uint64_t remaining() const {
        if (ticks <= vtime->clocks)
            return 0;
        else
            return ticks - vtime->clocks;
    }

    bool hasPassed() const {
        return UNLIKELY(ticks <= vtime->clocks);
    }

    bool hasPassed(uint64_t ref) const {
        return UNLIKELY(ref <= vtime->clocks);
    }

    uint64_t clock() const {
        return vtime->clocks;
    }
        
 private:
    uint64_t ticks;
    const VirtualTime *vtime;
};


class EventRateProbe {
public:
    EventRateProbe(uint32_t counter=0)
        : counter(counter), hz(0) {}
        
    void update(ElapsedTime &et, uint32_t newValue) {
        uint32_t oldValue = counter;
        double s = et.virtualSeconds();
        counter = newValue;
        hz = s > 0 ? (newValue - oldValue) / s : 0;
    }
    
    float getHZ() {
        return hz;
    }

private:
    uint32_t counter;
    float hz;
};
   

#endif
