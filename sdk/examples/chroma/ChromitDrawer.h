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
};

#endif
