/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBE_NEIGHBORS_H
#define _CUBE_NEIGHBORS_H

#include <stdint.h>

#include "cube_cpu_reg.h"
#include "vtime.h"

namespace Cube {

class Hardware;


/*
 * Our neighbor sensors can transmit or receive small inductive
 * pulses.  In the new hardware design for Thundercracker, all
 * receivers share a single amplifier. So, the signal we receive is
 * more like a logical OR of the pulses coming in from each side. This
 * signal drives an IRQ GPIO.
 *
 * We have one bidirectional GPIO pin per side which is used for dual
 * purposes. Edge transitions on these pins will resonate in the
 * transmitter's LC tank, sending a pulse to any nearby receiver. The
 * pins can also be used to squelch a single receiver channel. If the
 * pin is floating, the channel participates in the OR above. If the
 * pin is driven, the input is masked.
 *
 * XXX: This hardware simulation is very basic. Right now we just
 *      propagate pulses from transmitter to receiver in a totally
 *      idealized fashion. There are all sorts of analog electronic
 *      effects in this circuit which can make it sensitive to changes
 *      in the firmware. Especially pulse width (must resonate with
 *      the LC tank) and pulse spacing, and the possibility of
 *      detecting false/reflected pulses.
 */

class Neighbors {
 public:
    enum Side {
        TOP = 0,
        RIGHT,
        BOTTOM,
        LEFT,
        NUM_SIDES,
    };

    void init() {
        memset(&mySides, 0, sizeof mySides);
    };
    
    void attachCubes(Hardware *cubes);

    void setContact(unsigned mySide, unsigned otherSide, unsigned otherCube) {
        /* Mark two neighbor sensors as in-range. ONLY called by the UI thread. */
        mySides[mySide].otherSides[otherSide] |= 1 << otherCube;
    }

    void clearContact(unsigned mySide, unsigned otherSide, unsigned otherCube) {
        /* Mark two neighbor sensors as not-in-range. ONLY called by the UI thread. */
        mySides[mySide].otherSides[otherSide] &= ~(1 << otherCube);
    }

    void tick(uint8_t *regs) {
        static const uint8_t outPinLUT[] = { PIN_OUT1, PIN_OUT2, PIN_OUT3, PIN_OUT4 };

        if (!otherCubes) {
            // So lonely...
            return;
        }

        uint8_t drive = ~regs[DIR];               // Pins we're driving at all
        uint8_t driveHigh = drive & regs[PORT];   // Pins we're driving high
        uint8_t in = 0;

        // Rising driven-edge detector. This is our pulse transmit state.
        driveEdge = driveHigh & ~prevDriveHigh;
        prevDriveHigh = driveHigh;

        if (drive & PIN_IN) {
            // Input pin is being driven. No use detecting its input state...
            return;
        }

        // OR together every non-masked RX sensor
        for (unsigned mySide = 0; mySide < NUM_SIDES; mySide++) {

            // Is this receiver non-masked?
            if (!(drive & outPinLUT[mySide])) {

                // Look at each other-side bitmask...
                for (unsigned otherSide = 0; otherSide < NUM_SIDES; otherSide++) {
                    uint32_t sideMask = mySides[mySide].otherSides[otherSide];
                    uint8_t otherSideBit = outPinLUT[otherSide];

                    /*
                     * Loop over all neighbored cubes on this
                     * side. Should just be zero or one cubes, but
                     * it's easy to stay generic here.
                     */

                    while (sideMask) {
                        int otherCube = __builtin_ffs(sideMask) - 1;
                        in |= checkNeighbor(otherCube, otherSideBit);
                        sideMask ^= 1 << otherCube;
                    }
                }
            }
        }

        // Update state of input pin
        regs[PORT] = (regs[PORT] & ~PIN_IN) | in;
    }

 private:
    uint8_t checkNeighbor(unsigned otherCube, uint8_t otherSideBit);

    static const unsigned PORT     = REG_P1;
    static const unsigned DIR      = REG_P1DIR;
    static const unsigned PIN_IN   = (1 << 4);
    static const unsigned PIN_OUT1 = (1 << 1);
    static const unsigned PIN_OUT2 = (1 << 5);
    static const unsigned PIN_OUT3 = (1 << 6);
    static const unsigned PIN_OUT4 = (1 << 7);
    
    uint8_t driveEdge;
    uint8_t prevDriveHigh;

    struct {
        uint32_t otherSides[NUM_SIDES];
    } mySides[NUM_SIDES];

    Hardware *otherCubes;
};


};  // namespace Cube

#endif
