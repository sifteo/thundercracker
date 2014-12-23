/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Micah Elizabeth Scott <micah@misc.name>
 *
 * Copyright <c> 2012 Sifteo, Inc.
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

#include "mc_neighbor.h"
#include "neighbor_protocol.h"
#include "system.h"
#include "system_mc.h"

MCNeighbor::CubeInfo MCNeighbor::cubes[MCNeighbor::NUM_SIDES];
TickDeadline MCNeighbor::deadline;
uint32_t MCNeighbor::nbrSides;
uint32_t MCNeighbor::txSides;
uint32_t MCNeighbor::txData;
uint32_t MCNeighbor::txRegister;


void MCNeighbor::updateNeighbor(bool touching, unsigned mcSide, unsigned cube, unsigned cubeSide)
{
    ASSERT(mcSide < NUM_SIDES);

    if (touching) {
        MCNeighbor::cubes[mcSide].id = cube;
        MCNeighbor::cubes[mcSide].side = cubeSide;
        MCNeighbor::nbrSides |= 1 << mcSide;
    } else {
        MCNeighbor::nbrSides &= ~(1 << mcSide);
    }

    // Wake up virtual hardware simulation
    deadline.set(0);
}

void NeighborTX::init()
{
    stop();
}

void NeighborTX::start(unsigned data, unsigned sideMask)
{
    MCNeighbor::txData = data;
    MCNeighbor::txSides = sideMask;

    // Wake up virtual hardware simulation
    MCNeighbor::deadline.set(0);
}

void NeighborTX::stop()
{
    MCNeighbor::txData = 0;
    MCNeighbor::txSides = 0;
}

void MCNeighbor::deadlineWork()
{
    /*
     * Tick callback, called on the cube simulation thread by SystemCubes.
     * We simulate the timing of the master's neighbor transmitters, and
     * broadcast the resulting pulses to any cubes who are listening.
     */

    deadline.reset();

    // Early out without rescheduling, if nobody is here to listen.
    unsigned sides = txSides & nbrSides;
    if (!sides)
        return;
    
    // Out of bits? Refill, and wait for the next transmit period.
    if (!txRegister) {
        txRegister = txData << 16;
        deadline.setRelative(Neighbor::NUM_TX_WAIT_PERIODS * Neighbor::BIT_PERIOD_TICKS
            * VirtualTime::HZ / Neighbor::TICK_HZ);
        return;
    }

    // Are we transmitting a pulse?
    if (txRegister >> 31)
        for (unsigned i = 0; i < NUM_SIDES; ++i)
            if (sides & (1 << i))
                transmitPulse(cubes[i]);

    // Wait for the next bit
    txRegister <<= 1;
    deadline.setRelative(Neighbor::BIT_PERIOD_TICKS * VirtualTime::HZ / Neighbor::TICK_HZ);
}

void MCNeighbor::transmitPulse(const CubeInfo &dest)
{
    Cube::Hardware &hw = SystemMC::getSystem()->cubes[dest.id];

    if (hw.neighbors.isSideReceiving(dest.side))
        hw.neighbors.receivedPulse(hw.cpu);
}
