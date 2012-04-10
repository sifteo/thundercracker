/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Save load data structure and functionality
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SAVELOAD_H
#define _SAVELOAD_H

#include <sifteo.h>

using namespace Sifteo;

struct SaveData
{
    static const unsigned int NUM_HIGH_SCORES = 5;

    uint8_t furthestProgress;
    uint8_t lastPlayedPuzzle;

    unsigned int aHighScores[ NUM_HIGH_SCORES ];
    unsigned int aHighCubes[ NUM_HIGH_SCORES ];

    void InitDefaultValues();
    void Load();
    void Save();
};

#endif
