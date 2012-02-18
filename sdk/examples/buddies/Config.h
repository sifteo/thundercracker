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
    GAME_MODE_PUZZLE,
    
    NUM_GAME_MODES
};

enum GameState
{
    GAME_STATE_NONE = 0,
    GAME_STATE_FREE_PLAY,
    GAME_STATE_SHUFFLE_START,
    GAME_STATE_SHUFFLE_SHAKE_TO_SCRAMBLE,
    GAME_STATE_SHUFFLE_SCRAMBLING,
    GAME_STATE_SHUFFLE_UNSCRAMBLE_THE_FACES,
    GAME_STATE_SHUFFLE_PLAY,
    GAME_STATE_SHUFFLE_SOLVED,
    GAME_STATE_SHUFFLE_SCORE,
    GAME_STATE_PUZZLE_START,
    GAME_STATE_PUZZLE_INSTRUCTIONS,
    GAME_STATE_PUZZLE_PLAY,
    GAME_STATE_PUZZLE_SOLVED,
    
    NUM_GAME_STATES
};

const bool kLoadAssets = false;
const GameMode kGameMode = GAME_MODE_SHUFFLE;

const unsigned int kNumCubes = 2; // Number of cubes used in this game
const unsigned int kMaxBuddies = 6; // Number of characters

const float kResetTimerDuration = 5.0f; // Touch a cube for this many seconds to reset the game
const float kHintTimerOnDuration = 10.0f; // Seconds before hint appears

const int kShuffleMaxMoves = -1; // Number of shuffles. -1 keeps going until all are shuffled.
const float kShuffleStateTimeDelay = 1.0f; // Delay when switching between shuffle states
const float kShuffleScrambleTimerDelay = 0.5f; // Time between end of swap animation and next

const int kSwapAnimationSpeed = 8; // Number of frames animated during swap on each update

const float kPieceAnimX = 0.0f;
const float kPieceAnimY = 0.0f;
const float kPieceAnimPeriod = 1.0f;

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
