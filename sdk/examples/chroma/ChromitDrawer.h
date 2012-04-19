/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Queues up draws for all chromits, then draws them
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _CHROMITDRAWER_H
#define _CHROMITDRAWER_H

#include <sifteo.h>
#include "config.h"
#include "cubewrapper.h"

using namespace Sifteo;

class ChromitDrawer
{
public:
    struct SlotInfo
    {
        const AssetImage *pImage;
        uint8_t frame;
        bool bDifferent;
    };

    ChromitDrawer();
    void Reset();

    //queue up a chromit to draw in this slot
    void queue( unsigned int cubeIndex, UByte2 pos, const AssetImage *pImg, uint8_t frame = 0 );
    void drawAll();

private:
    SlotInfo m_allInfo[ NUM_CUBES ][CubeWrapper::NUM_ROWS][CubeWrapper::NUM_COLS];

    struct MovingSlot
    {
        Int2 pos;
        SlotInfo slot;
    };

    //array of queued clears.
    //clears get queued up in update, then they get drawn before any draws and cleared out
    /*Int2 m_queuedClears[ NUM_CUBES ][NUM_ROWS * NUM_COLS];
    int m_numQueuedClears[ NUM_CUBES ];

    MovingSlot m_movingChromits[ NUM_CUBES ][NUM_ROWS * NUM_COLS];
    int m_numMovingChromits[ NUM_CUBES ];*/
};

#endif
