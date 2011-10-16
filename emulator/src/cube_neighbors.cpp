/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "cube_neighbors.h"
#include "cube_hardware.h"

namespace Cube {


void Neighbors::attachCubes(Cube::Hardware *cubes)
{
    /*
     * The system is giving us a pointer to the other cubes, so
     * that we can scan their neighbor state on demand.
     */ 
    otherCubes = cubes;
}

void Neighbors::ioTick(CPU::em8051 &cpu)
{
    static const uint8_t outPinLUT[] = { PIN_OUT1, PIN_OUT2, PIN_OUT3, PIN_OUT4 };

    if (!otherCubes) {
        // So lonely...
        return;
    }

    uint8_t inputPins = cpu.mSFR[DIR];                 // Which pins are in input mode?
    uint8_t driveHigh = cpu.mSFR[PORT] & ~inputPins;   // Pins we're driving high

    // Rising driven-edge detector. This is our pulse transmit state.
    uint8_t driveEdge = driveHigh & ~prevDriveHigh;
    prevDriveHigh = driveHigh;

    /*
     * Precalculate a mask of all sides that can receive right now.
     * If our input isn't working (e.g. its pin direction is wrong)
     * or if all sides are masked, this mask can be zero.
     */

    uint8_t nextMask = 0;

    if (inputPins & PIN_IN)
        for (unsigned s = 0; s < NUM_SIDES; s++)
            if (inputPins & outPinLUT[s])
                nextMask |= 1 << s;

    inputMask = nextMask;

    /*
     * Propagate outgoing edges to anyone who can hear them.
     * Edges are much much less frequent than clock ticks, so we
     * try to do as much of this work as possible on the transmit
     * side.
     */

    if (driveEdge) {
        for (unsigned mySide = 0; mySide < NUM_SIDES; mySide++) {
            uint8_t mySideBit = outPinLUT[mySide];

            if (mySideBit & driveEdge) {
                // We're transmitting on this side

                // If we're listening, we'll hear an echo
                inputs |= 0x80;

                // Look at each other-side bitmask...
                for (unsigned otherSide = 0; otherSide < NUM_SIDES; otherSide++) {
                    uint32_t sideMask = mySides[mySide].otherSides[otherSide];

                    /*
                     * Loop over all neighbored cubes on this
                     * side. Should just be zero or one cubes, but
                     * it's easy to stay generic here.
                     */

                    while (sideMask) {
                        int otherCube = __builtin_ffs(sideMask) - 1;
                        transmitPulse(cpu, otherCube, otherSide);
                        sideMask ^= 1 << otherCube;
                    }
                }
            }
        }
    }
}

void Neighbors::transmitPulse(CPU::em8051 &cpu, unsigned otherCube, uint8_t otherSide)
{
    uint8_t bit = 1 << otherSide;
    Neighbors &dest = otherCubes[otherCube].neighbors;

    if (dest.inputMask & bit) {
        if (cpu.traceFile)
            fprintf(cpu.traceFile, "[%2d] NEIGHBOR: Sending pulse to %d.%d\n",
                    cpu.id, otherCube, otherSide);

        dest.inputs |= bit;

    } else {
        if (cpu.traceFile)
            fprintf(cpu.traceFile, "[%2d] NEIGHBOR: Pulse to %d.%d was masked\n", 
                    cpu.id, otherCube, otherSide);
    }
}


};  // namespace Cube
