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

#include "GameState.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace Buddies {

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

const bool kLoadAssets = true;
const GameState kStateDefault = GAME_STATE_MENU_MAIN;
const unsigned int kNumCubes = 3; // Number of cubes used in this game
const unsigned int kMaxBuddies = 6; // Number of characters (eventually will be 15)
const unsigned int kPuzzlesPerBuddy = 5;
//#define BUDDY_PIECES_USE_SPRITES

// Tuning
const float kStateTimeDelayShort = 1.0f; // Delay when switching between shuffle states
const float kStateTimeDelayLong = 5.0f;
const float kOptionsTimerDuration = 1.5f;
const float kSwapAnimationSlide = 0.5f;
const float kHintTimerOnDuration = 30.0f; // Seconds before hint appears
const float kHintTimerOffDuration = 2.5f; // Seconds before hint disappears
const float kHintTimerRepeatDuration = 5.0f; // Seconds before hint reappears
const float kHintBlinkTimerDuration = 0.5f; // Blink rate for hints (tune this with kHintTimerOffDuration so the piece doesn't do a half-blink when turning off)
const float kClueTimerOnDuration = 30.0f;

// Free Play
const float kFreePlayShakeThrottleDuration = 1.5f;

// Shuffle Mode
const int kShuffleMaxMoves = -1; // Number of shuffles. -1 keeps going until all are shuffled.
const float kShuffleCharacterSplashDelay = 3.0f; // Time we see full buddy faces
const float kShuffleBannerSwapDelay = 3.0f; // Time between banner swaps
const float kShuffleScrambleTimerDelay = 0.1f; // Time between end of swap animation and next
const float kShuffleFaceCompleteTimerDelay = 3.0f; // Time before "Face Complete" is dismissed
const int kShuffleCutsceneJumpChance = 18; // The bigger the number, the slower they will jump

// Story Mode
const float kStoryCutsceneTextDelay = 2.5f;
const int kStoryCutsceneJumpChanceA = 8;
const int kStoryCutsceneJumpChanceB = 16;
const int kStoryUnlockJumpChance = 4;

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
