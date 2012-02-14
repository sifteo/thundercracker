/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SIFTEO_BUDDIES_CONFIG_H_
#define SIFTEO_BUDDIES_CONFIG_H_

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace Buddies {

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

enum GameMode
{
    GAME_MODE_FREE_PLAY = 0,
    GAME_MODE_SHUFFLE,
    GAME_MODE_AUTHORED,
    
    NUM_GAME_MODES
};

enum SwapState
{
    SWAP_STATE_NONE = 0,
    SWAP_STATE_OUT,
    SWAP_STATE_IN,
    
    NUM_SWAP_STATES
};

enum ShuffleState
{
    SHUFFLE_STATE_NONE = 0,
    SHUFFLE_STATE_START,
    SHUFFLE_STATE_SHAKE_TO_SCRAMBLE,
    SHUFFLE_STATE_SCRAMBLING,
    SHUFFLE_STATE_UNSCRAMBLE_THE_FACES,
    SHUFFLE_STATE_PLAY,
    SHUFFLE_STATE_SOLVED,
    SHUFFLE_STATE_SCORE,
    
    NUM_SHUFFLE_STATES
};

enum AuthoredState
{
    AUTHORED_STATE_NONE = 0,
    AUTHORED_STATE_START,
    AUTHORED_STATE_INSTRUCTIONS,
    AUTHORED_STATE_PLAY,
    AUTHORED_STATE_SOLVED,
    
    NUM_AUTHORED_STATES
};

const bool kLoadAssets = true;
const GameMode kGameMode = GAME_MODE_SHUFFLE;

const unsigned int kNumCubes = 2; // Number of cubes used in this game
const unsigned int kMaxBuddies = 6; // Number of characters

const float kResetTimerDuration = 3.0f; // Touch a cube for this many seconds to reset the game

const int kShuffleMaxMoves = -1; // Number of moves for each shuffle. -1 keeps it going until all are shuffled.
const float kShuffleStateTimeDelay = 1.0f; // Amount of delay when switching between shuffle states
const float kShuffleScrambleTimerDelay = 0.5f; // Time between end of swap animation and beginning of next

const int kSwapAnimationSpeed = 8; // Number of frames animated during swap on each update

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
