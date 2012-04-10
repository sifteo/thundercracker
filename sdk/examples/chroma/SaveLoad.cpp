/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Save load data structure and functionality
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SaveLoad.h"

/*
const unsigned int HIGHSCORE_PRESETS[ SaveData::NUM_HIGH_SCORES ] =
        { 1000, 800, 600, 400, 200 };

const unsigned int HIGHCUBE_PRESETS[ SaveData::NUM_HIGH_SCORES ] =
        { 20, 10, 8, 6, 4 };
*/

const unsigned int HIGHSCORE_PRESETS[ SaveData::NUM_HIGH_SCORES ] =
        { 200, 200, 200, 200, 200 };

const unsigned int HIGHCUBE_PRESETS[ SaveData::NUM_HIGH_SCORES ] =
        { 4, 4, 4, 4, 4 };


void SaveData::InitDefaultValues()
{
    furthestProgress = 0;
    lastPlayedPuzzle = 0;

    for( int i = 0; i < NUM_HIGH_SCORES; i++ )
    {
        aHighScores[i] = HIGHSCORE_PRESETS[i];
        aHighCubes[i] = HIGHCUBE_PRESETS[i];
    }

    for( int i = 0; i < NUM_UNLOCKS; i++ )
    {
        bUnlocks[i] = false;
    }
}


//TODO implement this!
void SaveData::Load()
{
    InitDefaultValues();
}

//TODO implement this!
void SaveData::Save()
{

}
