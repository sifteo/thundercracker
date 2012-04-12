/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Functionality surrounding Cube Buddy unlocking
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "assets.gen.h"
#include "CubeBuddy.h"
#include "Cubewrapper.h"
#include "game.h"
#include "SaveLoad.h"


const char *MODE_NAMES[SaveData::NUM_UNLOCKS] =
{
    "Survival",
    "Rush",
    "Puzzle",
};


static void WaitForTouch( CubeWrapper *cubes )
{
    bool touching = false;

    for( int i = 0; i < NUM_CUBES; i++ )
    {
        if( cubes[i].GetCube().touching() )
            touching = true;
    }

    LOG(("WaitForTouch"));

    cubes[0].GetCube().vbuf.touch();

    while( true )
    {
        System::paint();

        //wait for release and retouch
        if( touching )
        {          
            touching = false;

            for( int i = 0; i < NUM_CUBES; i++ )
            {
                if( cubes[i].GetCube().touching() )
                    touching = true;
            }
        }
        else
        {
            for( int i = 0; i < NUM_CUBES; i++ )
            {
                if( cubes[i].GetCube().touching() )
                    return;
            }
        }
    }
}

static void ShowProgress( const SaveData &data )
{
    LOG(("ShowProgress\n"));
    //take over control flow here
    CubeWrapper *cubes = Game::Inst().m_cubes;
    //clear out the other cubes
    for( int i = 1; i < NUM_CUBES; i++ )
    {
        cubes[i].GetVid().clear( GemEmpty.tiles[0] );
    }

    String<20> modes;
    int count = 0;

    for( int i = 0; i < SaveData::NUM_UNLOCKS; i++ )
    {
        if( !(data.bUnlocks[i]) )
        {
            if( count > 0 )
                modes << " and ";

            modes << MODE_NAMES[i];
            count++;
        }
    }

    String<64> str;
    str << "You've nearly freed Lumes! Play " << modes << " to free him!";

    LOG(("gonna draw ShowProgress"));

    cubes[0].ClearBG1();
    cubes[0].setDirty();
    cubes[0].DrawMessageBoxWithText( str );
    cubes[0].FlushBG1();

    WaitForTouch( cubes );
}


static void ShowUnlock()
{
    LOG(("ShowUnlock"));
    //take over control flow here
    CubeWrapper *cubes = Game::Inst().m_cubes;
    //clear out the other cubes
    for( int i = 1; i < NUM_CUBES; i++ )
    {
        cubes[i].GetVid().clear( GemEmpty.tiles[0] );
    }

    String<128> str;
    str << "Congrats!  Lumes unlocked.  Play Cube Buddies to see him in action!";

    cubes[0].ClearBG1();
    cubes[0].setDirty();
    cubes[0].DrawMessageBoxWithText( str );
    cubes[0].FlushBG1();

    WaitForTouch( cubes );
}



//trigger an unlock for the given mode
//returns whether we did something
bool ProcessUnlock( uint8_t mode )
{
    //did we already do this unlock?
    SaveData &data = Game::Inst().getSaveData();

    //TODO, possibly send a reminder to the user?
    if( data.bUnlocks[mode] )
        return false;

    LOG(("Processing unlock"));

    data.bUnlocks[mode] = true;
    data.Save();

    for( int i = 0; i < SaveData::NUM_UNLOCKS; i++ )
    {
        LOG(("is mode unlocked? %d\n", data.bUnlocks[mode]));
        if( !data.bUnlocks[i] )
        {
            ShowProgress( data );
            return true;
        }
    }

    ShowUnlock();
    return true;
}
