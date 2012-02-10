/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "App.h"
#include <sifteo.h>
#include "Config.h"
#include "assets.gen.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace Buddies {

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

App::App()
    : mWrappers()
    , mChannel()
    , mResetTimer(0.0f)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::Setup()
{
    for (unsigned int i = 0; i < kNumCubes; ++i)
    {
        AddCube(i);
    }
    
    mChannel.init();
    
    mResetTimer = kResetTimerDuration;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::Tick(float dt)
{
    for (unsigned int i = 0; i < arraysize(mWrappers); ++i)
    {
        if (mWrappers[i].IsEnabled())
        {
            mWrappers[i].Tick();
        }
    }
    
    if (AnyTouching())
    {
        mResetTimer -= dt;
        
        if (mResetTimer <= 0.0f)
        {
            mResetTimer = kResetTimerDuration;
            
            for (unsigned int i = 0; i < arraysize(mWrappers); ++i)
            {
                if (mWrappers[i].IsEnabled())
                {
                    mWrappers[i].Reset();
                    mWrappers[i].Paint();
                }
            }
            
            mChannel.play(gems1_4A9);
        }
    }
    else
    {
        mResetTimer = kResetTimerDuration;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::Paint()
{
    for (unsigned int i = 0; i < arraysize(mWrappers); ++i)
    {
        if (mWrappers[i].IsEnabled())
        {
            mWrappers[i].Paint();
        }
    }
    
    System::paint();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::AddCube(Cube::ID cubeId)
{
    ASSERT(cubeId < arraysize(mWrappers));
    ASSERT(!mWrappers[cubeId].IsEnabled());
    
    mWrappers[cubeId].Enable(cubeId, cubeId % kMaxBuddies);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::RemoveCube(Cube::ID cubeId)
{
    ASSERT(cubeId < arraysize(mWrappers));
    ASSERT(mWrappers[cubeId].IsEnabled());
    
    mWrappers[cubeId].Disable();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

CubeWrapper &App::GetWrapper(Cube::ID cubeId)
{
    ASSERT(cubeId < arraysize(mWrappers));
    
    return mWrappers[cubeId];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::OnNeighborAdd(Cube::ID cubeId0, Cube::Side cubeSide0, Cube::ID cubeId1, Cube::Side cubeSide1)
{
    CubeWrapper &buddy0 = GetWrapper(cubeId0);
    CubeWrapper &buddy1 = GetWrapper(cubeId1);
    
    if (buddy0.GetMode() == BUDDY_MODE_HINT || buddy1.GetMode() == BUDDY_MODE_HINT)
    {
        return;
    }
    
    Piece piece0 = buddy0.GetPiece(cubeSide0);
    Piece piece1 = buddy1.GetPiece(cubeSide1);
    
    // Swap position, maintain rotation (most difficult)
    // piece0.mRotation = cube0Piece.mRotation + SIDE_ROTATIONS[cubeSide0][cubeSide1];
    // if (piece0.mRotation < 0) piece0.mRotation += 4;
    // piece1.mRotation = cube1Piece.mRotation + SIDE_ROTATIONS[cubeSide1][cubeSide0];
    // if (piece1.mRotation < 0) piece1.mRotation += 4;
    
    // Change rotation based on new position
    piece0.mRotation = cubeSide1 - piece0.mPart;
    if (piece0.mRotation < 0)
    {
        piece0.mRotation += 4;
    }
    
    piece1.mRotation = cubeSide0 - piece1.mPart;
    if (piece1.mRotation < 0)
    {
        piece1.mRotation += 4;
    }
    
    // Swap pieces
    buddy0.SetPiece(cubeSide0, piece1);
    buddy1.SetPiece(cubeSide1, piece0);
    
    // If we have solved a face, play the sound.
    if (buddy0.IsSolved() || buddy1.IsSolved())
    {
        mChannel.play(gems1_4A9);
    }
    
    // Paint
    buddy0.Paint();
    buddy1.Paint();
    System::paint();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool App::AnyTouching() const
{
    for (unsigned int i = 0; i < arraysize(mWrappers); ++i)
    {
        if (mWrappers[i].IsEnabled() && mWrappers[i].IsTouching())
        {
            return true;
        }
    }
    
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}
