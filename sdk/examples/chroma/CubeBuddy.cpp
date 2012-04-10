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
                    break;
            }
        }
    }
}

static void ShowProgress( const SaveData &data )
{
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
        if( !(data.bUnlocks[Game::Inst().getMode()]) )
        {
            if( count > 0 )
                modes << " and ";

            modes << MODE_NAMES[i];
        }
    }

    String<64> str;
    str << "You've nearly freed Lumes! Play " << modes << " to free him!";

    cubes[0].DrawMessageBoxWithText( str );

    WaitForTouch( cubes );
}


static void ShowUnlock()
{
    //take over control flow here
    CubeWrapper *cubes = Game::Inst().m_cubes;
    //clear out the other cubes
    for( int i = 1; i < NUM_CUBES; i++ )
    {
        cubes[i].GetVid().clear( GemEmpty.tiles[0] );
    }

    String<64> str;
    str << "Congratulations!  Lumes unlocked.  Play Cube Buddies to see him in action!";

    cubes[0].DrawMessageBoxWithText( str );

    WaitForTouch( cubes );
}



//trigger an unlock for the given mode
void ProcessUnlock( uint8_t mode )
{
    //did we already do this unlock?
    SaveData &data = Game::Inst().getSaveData();

    //TODO, possibly send a reminder to the user?
    if( data.bUnlocks[mode] )
        return;

    data.bUnlocks[mode] = true;
    data.Save();

    for( int i = 0; i < SaveData::NUM_UNLOCKS; i++ )
    {
        if( !data.bUnlocks[mode] )
        {
            ShowProgress( data );
            break;
        }
    }

    ShowUnlock();
}
